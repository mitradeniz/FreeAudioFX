#include "EffectAmpEngine.h"

namespace oap::amps
{

using namespace oap::dsp;

namespace
{
    double dbToGain (double db) noexcept { return std::pow (10.0, db / 20.0); }

    double knobToDb (double knob, double minDb, double maxDb) noexcept
    {
        return minDb + (maxDb - minDb) * juce::jlimit (0.0, 10.0, knob) / 10.0;
    }

    void applyFixed (fx::Effect& e, const BuiltInEffect& def)
    {
        std::vector<fx::Effect*> one { &e };
        EffectSetter s (one);
        for (const auto& [id, value] : def.fixed)
            s.set (0, id.c_str(), value);
    }
}

//==============================================================================
EffectAmpEngine::EffectAmpEngine (const AmpModel& m)
    : model (m), voicing (m.voicing)
{
    jassert (! voicing.stages.empty() && voicing.stages.size() <= (size_t) maxStages);
    jassert (model.fxControls.size() <= (size_t) maxFxControls);

    numStages = juce::jlimit (1, maxStages, (int) voicing.stages.size());

    for (const auto& def : model.effects)
    {
        auto e = fx::createEffect (def.effectId);
        jassert (e != nullptr); // unknown effect id in the model
        if (e != nullptr)
            applyFixed (*e, def);
        effectPtrs.push_back (e.get());
        effects.push_back (std::move (e));
    }
}

EffectAmpEngine::~EffectAmpEngine() = default;

EffectAmpEngine::Params EffectAmpEngine::defaultParams() const
{
    Params p;
    p.gateDb  = voicing.hasGate ? voicing.gateDefaultDb : (float) NoiseGate::offThresholdDb;
    p.gain    = model.defaultGain;
    p.bass    = model.defaultBass;
    p.middle  = model.defaultMiddle;
    p.treble  = model.defaultTreble;
    p.master  = model.defaultMaster;
    p.cabinet = voicing.defaultCabinet;
    for (size_t i = 0; i < model.fxControls.size() && i < p.fx.size(); ++i)
        p.fx[i] = model.fxControls[i].def;
    return p;
}

//==============================================================================
void EffectAmpEngine::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    const int order = juce::jlimit (1, 3, voicing.oversamplingOrder);
    osRate = sampleRate * (double) (1 << order);
    maxBlock = juce::jmax (1, maxBlockSize);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        1, (size_t) order, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampler->initProcessing ((size_t) chunkSize);

    gate.prepare (sampleRate);

    // Linear voicing at the base rate
    inputHighpass.setCoeffs (BiquadCoeffs::highpass (sampleRate, voicing.inputHighpassHz, 0.707));
    useBright = std::abs (voicing.brightDb) > 0.01;
    bright.setCoeffs (BiquadCoeffs::highShelf (sampleRate, voicing.brightHz, 0.6, voicing.brightDb));
    usePreMid = std::abs (voicing.preMidDb) > 0.01;
    preMid.setCoeffs (BiquadCoeffs::peak (sampleRate, voicing.preMidHz, voicing.preMidQ, voicing.preMidDb));

    // Non-linear amp at the oversampled rate
    for (int i = 0; i < numStages; ++i)
        stages[(size_t) i].prepare (osRate, voicing.stages[(size_t) i]);

    toneStack.prepare (osRate, voicing.toneStack);

    usePostMid = std::abs (voicing.postMidDb) > 0.01;
    postMid.setCoeffs (BiquadCoeffs::peak (osRate, voicing.postMidHz, voicing.postMidQ, voicing.postMidDb));
    useLowShelf = std::abs (voicing.lowShelfDb) > 0.01;
    lowShelf.setCoeffs (BiquadCoeffs::lowShelf (osRate, voicing.lowShelfHz, 0.7, voicing.lowShelfDb));
    usePresence = std::abs (voicing.presenceDb) > 0.01;
    presence.setCoeffs (BiquadCoeffs::highShelf (osRate, 3500.0, 0.6, voicing.presenceDb));
    powerBandwidth.setCoeffs (BiquadCoeffs::lowpass (osRate, juce::jmin (voicing.powerBandwidthHz, 0.2 * osRate), 0.707));

    powerAmp.prepare (osRate);
    powerAmp.setSag (voicing.sag);

    cabinet.prepare (sampleRate, maxBlock);

    for (auto& e : effects)
        if (e != nullptr)
            e->prepare (sampleRate, maxBlock);

    for (auto* s : { &gain, &bass, &middle, &treble, &master })
        s->reset (sampleRate, 0.05);
    outputGain.reset (sampleRate, 0.05);

    reset();
}

void EffectAmpEngine::reset()
{
    if (oversampler != nullptr)
        oversampler->reset();

    gate.reset();

    for (auto* f : { &inputHighpass, &bright, &preMid, &postMid, &lowShelf, &presence, &powerBandwidth })
        f->reset();

    for (auto& s : stages)
        s.reset();

    toneStack.reset();
    powerAmp.reset();
    cabinet.reset();

    for (auto& e : effects)
        if (e != nullptr)
            e->reset();

    appliedBass = appliedMiddle = appliedTreble = -1.0f;
    snapControls = true;
}

void EffectAmpEngine::setTempo (double bpm) noexcept
{
    for (auto& e : effects)
        if (e != nullptr)
            e->setTempo (bpm);
}

int EffectAmpEngine::getLatencySamples() const noexcept
{
    int latency = oversampler != nullptr ? juce::roundToInt (oversampler->getLatencyInSamples()) : 0;
    for (const auto& e : effects)
        if (e != nullptr)
            latency += e->getLatencySamples();
    return latency;
}

double EffectAmpEngine::getTailSeconds() const noexcept
{
    double tail = 0.3; // cabinet and amp ring-out
    for (const auto& e : effects)
        if (e != nullptr)
            tail = juce::jmax (tail, e->getTailSeconds());
    return tail;
}

//==============================================================================
void EffectAmpEngine::setTargets (const Params& p) noexcept
{
    const auto set = [this] (auto& smoother, float value)
    {
        if (snapControls)
            smoother.setCurrentAndTargetValue (value);
        else
            smoother.setTargetValue (value);
    };

    set (gain, juce::jlimit (0.0f, 10.0f, p.gain));
    set (bass, juce::jlimit (0.0f, 10.0f, p.bass));
    set (middle, juce::jlimit (0.0f, 10.0f, p.middle));
    set (treble, juce::jlimit (0.0f, 10.0f, p.treble));
    set (master, juce::jlimit (0.0f, 10.0f, p.master));
    set (outputGain, (float) dbToGain (juce::jlimit (-60.0f, 24.0f, p.outputDb)));

    gate.setThresholdDb (voicing.hasGate ? p.gateDb : NoiseGate::offThresholdDb);
    cabinet.setType (p.cabinet);

    if (model.applyControls)
        model.applyControls (p.fx.data(), setter);

    snapControls = false;
}

void EffectAmpEngine::advanceControls (int numSamples) noexcept
{
    const float b = bass.skip (numSamples), m = middle.skip (numSamples), t = treble.skip (numSamples);
    if (hasChanged (b, appliedBass) || hasChanged (m, appliedMiddle) || hasChanged (t, appliedTreble))
    {
        toneStack.setControls (b / 10.0, m / 10.0, t / 10.0);
        appliedBass = b; appliedMiddle = m; appliedTreble = t;
    }

    interstageGain = dbToGain (knobToDb (gain.skip (numSamples), voicing.gainMinDb, voicing.gainMaxDb));
    powerDrive = dbToGain (knobToDb (master.skip (numSamples), voicing.masterMinDb, voicing.masterMaxDb));
    powerMakeup = voicing.outputTrim / std::pow (powerDrive, voicing.masterCompensation);
    if (voicing.gainCompensation > 0.0)
        powerMakeup *= std::pow (interstageGain, -voicing.gainCompensation);
}

//==============================================================================
double EffectAmpEngine::processOversampled (double x) noexcept
{
    double v = stages[0].process (x) * interstageGain;
    for (int i = 1; i < numStages; ++i)
        v = stages[(size_t) i].process (v);

    v = toneStack.process (v) * voicing.toneMakeup;
    if (usePostMid)  v = postMid.process (v);
    if (useLowShelf) v = lowShelf.process (v);
    if (usePresence) v = presence.process (v);
    v = powerBandwidth.process (v);

    return powerAmp.process (v * powerDrive) * powerMakeup;
}

void EffectAmpEngine::runEffects (bool preAmp, float* const* channels, int numChannels, int numSamples) noexcept
{
    for (size_t i = 0; i < effects.size(); ++i)
    {
        auto* e = effects[i].get();
        if (e == nullptr || model.effects[i].beforeAmp != preAmp)
            continue;

        for (int start = 0; start < numSamples; start += maxBlock)
        {
            const int len = juce::jmin (maxBlock, numSamples - start);
            float* piece[2] = { channels[0] + start, numChannels > 1 ? channels[1] + start : nullptr };
            e->process (piece, numChannels, len);
        }
    }
}

void EffectAmpEngine::process (juce::AudioBuffer<float>& buffer, int numInputChannels, const Params& params) noexcept
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0 || oversampler == nullptr)
        return;

    // The amp is mono: average the input channels into channel 0.
    numInputChannels = juce::jlimit (1, numChannels, numInputChannels);
    for (int ch = 1; ch < numInputChannels; ++ch)
        buffer.addFrom (0, 0, buffer, ch, 0, numSamples);
    if (numInputChannels > 1)
        buffer.applyGain (0, 0, numSamples, 1.0f / (float) numInputChannels);

    float* mono = buffer.getWritePointer (0);

    setTargets (params);
    gate.process (mono, numSamples);

    {
        float* pre[] = { mono };
        runEffects (true, pre, 1, numSamples);
    }

    for (int start = 0; start < numSamples; start += chunkSize)
    {
        const int len = juce::jmin (chunkSize, numSamples - start);
        advanceControls (len);

        float* x = mono + start;

        for (int i = 0; i < len; ++i)
        {
            double v = inputHighpass.process (x[i]);
            if (useBright) v = bright.process (v);
            if (usePreMid) v = preMid.process (v);
            x[i] = (float) v;
        }

        float* channels[] = { x };
        juce::dsp::AudioBlock<float> block (channels, 1, (size_t) len);
        auto up = oversampler->processSamplesUp (block);
        float* u = up.getChannelPointer (0);

        for (size_t i = 0, n = up.getNumSamples(); i < n; ++i)
            u[i] = (float) processOversampled (u[i]);

        oversampler->processSamplesDown (block);
    }

    for (int start = 0; start < numSamples; start += maxBlock)
        cabinet.process (mono + start, juce::jmin (maxBlock, numSamples - start));

    // Stereo effects after the cabinet, like a studio rack or a stereo amp's effects loop.
    const int fxChannels = juce::jmin (2, numChannels);
    if (fxChannels > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    runEffects (false, buffer.getArrayOfWritePointers(), fxChannels, numSamples);

    double energy = 0.0;
    float* left = buffer.getWritePointer (0);
    float* right = fxChannels > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGain.getNextValue();
        left[i] *= g;
        energy += (double) left[i] * left[i];
        if (right != nullptr)
        {
            right[i] *= g;
            energy += (double) right[i] * right[i];
        }
    }

    // Never let a bad state escape to the host.
    if (! std::isfinite (energy))
    {
        buffer.clear();
        reset();
        return;
    }

    for (int ch = fxChannels; ch < numChannels; ++ch)
        buffer.copyFrom (ch, 0, buffer, ch % 2, 0, numSamples);
}

} // namespace oap::amps
