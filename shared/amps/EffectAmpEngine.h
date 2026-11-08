#pragma once

#include <juce_dsp/juce_dsp.h>

#include "amps/AmpModel.h"
#include "dsp/OapDsp.h"

#include <array>

namespace oap::amps
{

//==============================================================================
/** The signal path shared by every effect amp, configured by an AmpModel:

    gate -> [pre effects, e.g. fuzz] -> input HPF -> bright -> pre-mid ->
    ( oversampled: triode stages (Gain between 1 and 2) -> tone stack ->
      post-mid -> low shelf -> presence -> power amp ) ->
    cabinet -> [post effects in stereo: modulation, delay, reverb] -> output

    The amp section is mono like a real amp; the effects after it run in
    stereo when the output bus has two channels. Independent of the plugin
    wrapper so tests can drive it directly. */
class EffectAmpEngine
{
public:
    static constexpr int maxFxControls = 5;

    struct Params
    {
        float gateDb   = -70.0f; // ignored when the model has no gate; <= -90 = off
        float gain     = 5.0f;   // knobs are 0..10
        float bass     = 5.0f;
        float middle   = 5.0f;
        float treble   = 5.0f;
        float master   = 5.0f;
        float outputDb = 0.0f;
        int   cabinet  = dsp::CabinetSection::Closed4x12;
        std::array<float, maxFxControls> fx {};
    };

    explicit EffectAmpEngine (const AmpModel&);
    ~EffectAmpEngine();

    /** Parameters at the model's defaults. */
    Params defaultParams() const;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setTempo (double bpm) noexcept;

    /** Processes the first numInputChannels as input and fills all channels of the buffer. */
    void process (juce::AudioBuffer<float>& buffer, int numInputChannels, const Params& params) noexcept;

    int getLatencySamples() const noexcept;
    double getTailSeconds() const noexcept;

    const AmpModel& getModel() const noexcept { return model; }
    dsp::CabinetSection& getCabinet() noexcept { return cabinet; }

private:
    void setTargets (const Params&) noexcept;
    void advanceControls (int numSamples) noexcept;
    double processOversampled (double x) noexcept;
    void runEffects (bool preAmp, float* const* channels, int numChannels, int numSamples) noexcept;

    static constexpr int chunkSize = 32;
    static constexpr int maxStages = 4;

    const AmpModel& model;
    const AmpVoicing& voicing;

    double sampleRate = 48000.0, osRate = 192000.0;
    int maxBlock = 512;
    bool snapControls = true;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    dsp::NoiseGate gate;
    dsp::Biquad inputHighpass, bright, preMid;
    std::array<dsp::TubeStage, maxStages> stages;
    int numStages = 0;
    dsp::ToneStack toneStack;
    dsp::Biquad postMid, lowShelf, presence, powerBandwidth;
    bool useBright = false, usePreMid = false, usePostMid = false, useLowShelf = false, usePresence = false;
    dsp::PowerAmp powerAmp;
    dsp::CabinetSection cabinet;

    // Built-in effects, in model order; `effectPtrs` is what the FX mapping sees.
    std::vector<std::unique_ptr<fx::Effect>> effects;
    std::vector<fx::Effect*> effectPtrs;
    EffectSetter setter { effectPtrs };

    juce::SmoothedValue<float> gain, bass, middle, treble, master;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
    float appliedBass = -1.0f, appliedMiddle = -1.0f, appliedTreble = -1.0f;
    double interstageGain = 1.0, powerDrive = 1.0, powerMakeup = 1.0;

    JUCE_DECLARE_NON_COPYABLE (EffectAmpEngine)
};

} // namespace oap::amps
