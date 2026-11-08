#include "DriveAmpEngine.h"

using namespace oap::dsp;

namespace
{
    constexpr double kBoostLevel      = 2.5;  // +8 dB into the preamp when Boost is on
    constexpr double kToneStackMakeup = 4.0;  // recovers the passive tone stack's insertion loss
    constexpr double kOutputTrim      = 0.5;  // overall calibration: default peaks around -3 dBFS

    double dbToGain (double db) noexcept { return std::pow (10.0, db / 20.0); }

    /** Tight: 20 Hz .. 250 Hz, log-spaced across the knob. */
    double tightToHz (double knob) noexcept { return 20.0 * std::pow (250.0 / 20.0, knob / 10.0); }

    /** Gain pot between V1a and V1b: -36 dB .. +4 dB, linear in dB across the knob. */
    double gainToInterstage (double knob) noexcept { return dbToGain (-36.0 + 4.0 * knob); }

    /** Master: power amp drive -12 dB .. +12 dB. */
    double masterToDrive (double knob) noexcept { return dbToGain (-12.0 + 2.4 * knob); }
}

//==============================================================================
void DriveAmpEngine::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    osRate = sampleRate * (double) (1 << oversamplingOrder);
    maxBlock = juce::jmax (1, maxBlockSize);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        1, (size_t) oversamplingOrder, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, true);
    oversampler->initProcessing ((size_t) chunkSize);

    gate.prepare (sampleRate);

    brightVoicing.setCoeffs (BiquadCoeffs::highShelf (sampleRate, 1400.0, 0.6, 3.0));
    boostLow.setCoeffs  (BiquadCoeffs::lowShelf  (sampleRate, 220.0, 0.7, -9.0));
    boostMid.setCoeffs  (BiquadCoeffs::peak      (sampleRate, 850.0, 0.8, 5.0));
    boostHigh.setCoeffs (BiquadCoeffs::highShelf (sampleRate, 5000.0, 0.7, -3.0));

    //                       gain   bias  coupling  bandwidth
    stage1.prepare (osRate, { 6.0,  0.15,  30.0,    14000.0 }); // V1a: clean-ish input stage
    stage2.prepare (osRate, { 16.0, 0.45, 110.0,     9000.0 }); // V1b: cold-biased clipper
    stage3.prepare (osRate, { 2.0,  0.25,  70.0,     7500.0 }); // V2a (after a plate-load divider)
    stage4.prepare (osRate, { 1.2,  0.10,  35.0,     6500.0 }); // V2b / cathode follower

    toneStack.prepare (osRate, ToneStack::british());
    phaseInverterBandwidth.setCoeffs (BiquadCoeffs::lowpass (osRate, 11000.0, 0.707));
    powerAmp.prepare (osRate);
    powerAmp.setSag (0.25);

    cabinet.prepare (sampleRate, maxBlock);

    for (auto* s : { &tight, &gain, &bass, &middle, &treble, &presence, &resonance, &master, &boostMix })
        s->reset (sampleRate, 0.05);

    outputGain.reset (sampleRate, 0.05);

    reset();
}

void DriveAmpEngine::reset()
{
    if (oversampler != nullptr)
        oversampler->reset();

    gate.reset();

    for (auto* f : { &tightFilter, &brightVoicing, &boostLow, &boostMid, &boostHigh,
                     &resonanceFilter, &presenceFilter, &phaseInverterBandwidth })
        f->reset();

    for (auto* s : { &stage1, &stage2, &stage3, &stage4 })
        s->reset();

    toneStack.reset();
    powerAmp.reset();
    cabinet.reset();

    appliedTight = appliedBass = appliedMiddle = appliedTreble = appliedPresence = appliedResonance = -1.0f;
    snapControls = true;
}

int DriveAmpEngine::getLatencySamples() const noexcept
{
    return oversampler != nullptr ? juce::roundToInt (oversampler->getLatencyInSamples()) : 0;
}

//==============================================================================
void DriveAmpEngine::setTargets (const Params& p) noexcept
{
    const auto set = [this] (auto& smoother, float value)
    {
        if (snapControls)
            smoother.setCurrentAndTargetValue (value);
        else
            smoother.setTargetValue (value);
    };

    set (tight, juce::jlimit (0.0f, 10.0f, p.tight));
    set (gain, juce::jlimit (0.0f, 10.0f, p.gain));
    set (bass, juce::jlimit (0.0f, 10.0f, p.bass));
    set (middle, juce::jlimit (0.0f, 10.0f, p.middle));
    set (treble, juce::jlimit (0.0f, 10.0f, p.treble));
    set (presence, juce::jlimit (0.0f, 10.0f, p.presence));
    set (resonance, juce::jlimit (0.0f, 10.0f, p.resonance));
    set (master, juce::jlimit (0.0f, 10.0f, p.master));
    set (boostMix, p.boost ? 1.0f : 0.0f);
    set (outputGain, (float) dbToGain (juce::jlimit (-60.0f, 24.0f, p.outputDb)));

    gate.setThresholdDb (p.gateDb);
    cabinet.setType (p.cabinet);
    snapControls = false;
}

void DriveAmpEngine::advanceControls (int numSamples) noexcept
{
    if (const float v = tight.skip (numSamples); hasChanged (v, appliedTight))
    {
        tightFilter.setCoeffs (BiquadCoeffs::highpass (sampleRate, tightToHz (v), 0.707));
        appliedTight = v;
    }

    const float b = bass.skip (numSamples), m = middle.skip (numSamples), t = treble.skip (numSamples);
    if (hasChanged (b, appliedBass) || hasChanged (m, appliedMiddle) || hasChanged (t, appliedTreble))
    {
        toneStack.setControls (b / 10.0, m / 10.0, t / 10.0);
        appliedBass = b; appliedMiddle = m; appliedTreble = t;
    }

    if (const float v = presence.skip (numSamples); hasChanged (v, appliedPresence))
    {
        presenceFilter.setCoeffs (BiquadCoeffs::highShelf (osRate, 3200.0, 0.6, -6.0 + 1.5 * v));
        appliedPresence = v;
    }

    if (const float v = resonance.skip (numSamples); hasChanged (v, appliedResonance))
    {
        resonanceFilter.setCoeffs (BiquadCoeffs::peak (osRate, 95.0, 0.8, 1.0 * v));
        appliedResonance = v;
    }

    interstageGain = gainToInterstage (gain.skip (numSamples));
    powerDrive = masterToDrive (master.skip (numSamples));
    powerMakeup = kOutputTrim / std::pow (powerDrive, 0.12);
    boostAmount = boostMix.skip (numSamples);
}

//==============================================================================
double DriveAmpEngine::processOversampled (double x) noexcept
{
    double v = stage1.process (x);
    v = stage2.process (v * interstageGain);
    v = stage3.process (v);
    v = stage4.process (v);

    v = toneStack.process (v) * kToneStackMakeup;
    v = resonanceFilter.process (v);
    v = presenceFilter.process (v);
    v = phaseInverterBandwidth.process (v);

    return powerAmp.process (v * powerDrive) * powerMakeup;
}

void DriveAmpEngine::process (juce::AudioBuffer<float>& buffer, int numInputChannels, const Params& params) noexcept
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

    for (int start = 0; start < numSamples; start += chunkSize)
    {
        const int len = juce::jmin (chunkSize, numSamples - start);
        advanceControls (len);

        float* x = mono + start;

        // Linear pre-EQ at the base rate.
        for (int i = 0; i < len; ++i)
        {
            double v = tightFilter.process (x[i]);
            v = brightVoicing.process (v);
            const double boosted = boostHigh.process (boostMid.process (boostLow.process (v))) * kBoostLevel;
            x[i] = (float) (v + boostAmount * (boosted - v));
        }

        // Non-linear amp at 8x.
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

    double energy = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        mono[i] *= outputGain.getNextValue();
        energy += (double) mono[i] * mono[i];
    }

    // Never let a bad state escape to the host.
    if (! std::isfinite (energy))
    {
        buffer.clear();
        reset();
        return;
    }

    for (int ch = 1; ch < numChannels; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}
