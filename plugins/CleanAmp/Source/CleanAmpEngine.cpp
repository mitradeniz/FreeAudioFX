#include "CleanAmpEngine.h"

using namespace oap::dsp;

namespace
{
    constexpr double kToneStackMakeup = 4.0;  // recovers the passive tone stack's insertion loss
    constexpr double kOutputTrim      = 0.7;  // overall calibration: default peaks around -4 dBFS

    double dbToGain (double db) noexcept { return std::pow (10.0, db / 20.0); }

    /** Volume pot after the tone stack: -30 dB .. +12 dB. Turning it up drives V2 into break-up. */
    double volumeToGain (double knob) noexcept { return dbToGain (-30.0 + 4.2 * knob); }

    /** Master: power amp drive -12 dB .. +14 dB. */
    double masterToDrive (double knob) noexcept { return dbToGain (-12.0 + 2.6 * knob); }

    /** The bright cap bypasses the volume pot, so its boost fades out as the volume goes up. */
    double brightBoostDb (double mix, double volumeKnob) noexcept { return mix * 9.0 * (1.0 - volumeKnob / 10.0); }
}

//==============================================================================
void CleanAmpEngine::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    osRate = sampleRate * (double) (1 << oversamplingOrder);
    maxBlock = juce::jmax (1, maxBlockSize);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        1, (size_t) oversamplingOrder, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, true);
    oversampler->initProcessing ((size_t) chunkSize);

    //                       gain  bias  coupling  bandwidth
    stage1.prepare (osRate, { 2.0, 0.10,  20.0,    18000.0 }); // V1: hi-fi input stage
    stage2.prepare (osRate, { 3.0, 0.18,  25.0,    12000.0 }); // V2: breaks up when the volume is high

    toneStack.prepare (osRate, ToneStack::blackface());
    powerAmp.prepare (osRate);
    powerAmp.setSag (0.9);

    cabinet.prepare (sampleRate, maxBlock);

    reverbBuffer.setSize (2, maxBlock);
    reverb.setSampleRate (sampleRate);
    for (int ch = 0; ch < 2; ++ch)
    {
        reverbHighpass[ch].setCoeffs (BiquadCoeffs::highpass (sampleRate, 220.0, 0.707));
        reverbLowpass[ch].setCoeffs (BiquadCoeffs::lowpass (sampleRate, 5500.0, 0.707));
    }

    for (auto* s : { &volume, &bass, &middle, &treble, &master, &brightMix })
        s->reset (sampleRate, 0.05);

    outputGain.reset (sampleRate, 0.05);

    reset();
}

void CleanAmpEngine::reset()
{
    if (oversampler != nullptr)
        oversampler->reset();

    brightFilter.reset();
    stage1.reset();
    stage2.reset();
    toneStack.reset();
    powerAmp.reset();
    cabinet.reset();
    reverb.reset();

    for (int ch = 0; ch < 2; ++ch)
    {
        reverbHighpass[ch].reset();
        reverbLowpass[ch].reset();
    }

    appliedBright = appliedBass = appliedMiddle = appliedTreble = -1.0f;
    reverbAmount = -1.0f;
    snapControls = true;
}

int CleanAmpEngine::getLatencySamples() const noexcept
{
    return oversampler != nullptr ? juce::roundToInt (oversampler->getLatencyInSamples()) : 0;
}

//==============================================================================
void CleanAmpEngine::setTargets (const Params& p) noexcept
{
    const auto set = [this] (auto& smoother, float value)
    {
        if (snapControls)
            smoother.setCurrentAndTargetValue (value);
        else
            smoother.setTargetValue (value);
    };

    set (volume, juce::jlimit (0.0f, 10.0f, p.volume));
    set (bass, juce::jlimit (0.0f, 10.0f, p.bass));
    set (middle, juce::jlimit (0.0f, 10.0f, p.middle));
    set (treble, juce::jlimit (0.0f, 10.0f, p.treble));
    set (master, juce::jlimit (0.0f, 10.0f, p.master));
    set (brightMix, p.bright ? 1.0f : 0.0f);
    set (outputGain, (float) dbToGain (juce::jlimit (-60.0f, 24.0f, p.outputDb)));

    if (const float r = juce::jlimit (0.0f, 10.0f, p.reverb); hasChanged (r, reverbAmount))
    {
        reverbAmount = r;
        juce::Reverb::Parameters rp;
        rp.roomSize   = 0.62f;
        rp.damping    = 0.45f;
        rp.width      = 1.0f;
        rp.dryLevel   = 0.0f;                  // the dry signal is added separately
        rp.wetLevel   = 0.22f * (r / 10.0f);   // juce::Reverb scales wet by 3 internally
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
    }

    cabinet.setType (p.cabinet);
    snapControls = false;
}

void CleanAmpEngine::advanceControls (int numSamples) noexcept
{
    const float vol = volume.skip (numSamples);
    const float bright = (float) brightBoostDb (brightMix.skip (numSamples), vol);

    if (hasChanged (bright, appliedBright))
    {
        brightFilter.setCoeffs (BiquadCoeffs::highShelf (sampleRate, 1800.0, 0.7, bright));
        appliedBright = bright;
    }

    const float b = bass.skip (numSamples), m = middle.skip (numSamples), t = treble.skip (numSamples);
    if (hasChanged (b, appliedBass) || hasChanged (m, appliedMiddle) || hasChanged (t, appliedTreble))
    {
        toneStack.setControls (b / 10.0, m / 10.0, t / 10.0);
        appliedBass = b; appliedMiddle = m; appliedTreble = t;
    }

    volumeGain = volumeToGain (vol);
    powerDrive = masterToDrive (master.skip (numSamples));

    // Partial loudness compensation: Volume and Master still get louder, but by
    // roughly half as much as they increase the drive.
    powerMakeup = kOutputTrim / (std::pow (powerDrive, 0.45) * std::pow (volumeGain, 0.35));
}

//==============================================================================
double CleanAmpEngine::processOversampled (double x) noexcept
{
    double v = stage1.process (x);
    v = toneStack.process (v) * kToneStackMakeup;
    v = stage2.process (v * volumeGain);
    return powerAmp.process (v * powerDrive) * powerMakeup;
}

void CleanAmpEngine::processReverb (juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    const int numChannels = juce::jmin (2, buffer.getNumChannels());

    for (int start = 0; start < numSamples; start += maxBlock)
    {
        const int len = juce::jmin (maxBlock, numSamples - start);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* in = buffer.getReadPointer (ch, start);
            float* wet = reverbBuffer.getWritePointer (ch);

            for (int i = 0; i < len; ++i)
                wet[i] = (float) reverbLowpass[ch].process (reverbHighpass[ch].process (in[i]));
        }

        if (numChannels == 2)
            reverb.processStereo (reverbBuffer.getWritePointer (0), reverbBuffer.getWritePointer (1), len);
        else
            reverb.processMono (reverbBuffer.getWritePointer (0), len);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addFrom (ch, start, reverbBuffer, ch, 0, len);
    }
}

void CleanAmpEngine::process (juce::AudioBuffer<float>& buffer, int numInputChannels, const Params& params) noexcept
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0 || oversampler == nullptr)
        return;

    numInputChannels = juce::jlimit (1, numChannels, numInputChannels);
    for (int ch = 1; ch < numInputChannels; ++ch)
        buffer.addFrom (0, 0, buffer, ch, 0, numSamples);
    if (numInputChannels > 1)
        buffer.applyGain (0, 0, numSamples, 1.0f / (float) numInputChannels);

    float* mono = buffer.getWritePointer (0);

    setTargets (params);

    for (int start = 0; start < numSamples; start += chunkSize)
    {
        const int len = juce::jmin (chunkSize, numSamples - start);
        advanceControls (len);

        float* x = mono + start;

        for (int i = 0; i < len; ++i)
            x[i] = (float) brightFilter.process (x[i]);

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

    if (! std::isfinite (energy))
    {
        buffer.clear();
        reset();
        return;
    }

    for (int ch = 1; ch < numChannels; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    processReverb (buffer, numSamples);
}
