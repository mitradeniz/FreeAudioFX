#pragma once

#include <juce_dsp/juce_dsp.h>

#include "dsp/CabinetSection.h"
#include "dsp/OapDsp.h"

//==============================================================================
/** The complete Drive Amp signal path, independent of the plugin wrapper so it
    can be driven directly by tests.

    gate -> tight HPF -> [boost] -> bright voicing ->
    ( 8x oversampled: 4 triode stages -> tone stack -> resonance/presence -> phase inverter -> power amp ) ->
    cabinet -> output

    The amp itself is mono (like a real amp). Stereo input is summed to mono
    and the result is written to every output channel.
*/
class DriveAmpEngine
{
public:
    struct Params
    {
        float gateDb    = -75.0f; // <= -90 disables the gate
        float tight     = 5.0f;   // all knobs are 0..10
        bool  boost     = false;
        float gain      = 6.5f;
        float bass      = 5.0f;
        float middle    = 6.0f;
        float treble    = 6.0f;
        float presence  = 5.0f;
        float resonance = 4.0f;
        float master    = 5.0f;
        float outputDb  = 0.0f;
        int   cabinet   = oap::dsp::CabinetSection::Closed4x12;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Processes the first numInputChannels as input and fills all channels of the buffer. */
    void process (juce::AudioBuffer<float>& buffer, int numInputChannels, const Params& params) noexcept;

    int getLatencySamples() const noexcept;
    oap::dsp::CabinetSection& getCabinet() noexcept { return cabinet; }

    static constexpr int oversamplingOrder = 3; // 2^3 = 8x

private:
    void setTargets (const Params&) noexcept;
    void advanceControls (int numSamples) noexcept;
    double processOversampled (double x) noexcept;

    static constexpr int chunkSize = 32;

    double sampleRate = 48000.0, osRate = 384000.0;
    int maxBlock = 512;
    bool snapControls = true; // jump straight to the first parameter values after a reset
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    oap::dsp::NoiseGate gate;
    oap::dsp::Biquad tightFilter, brightVoicing, boostLow, boostMid, boostHigh;
    oap::dsp::TubeStage stage1, stage2, stage3, stage4;
    oap::dsp::ToneStack toneStack;
    oap::dsp::Biquad resonanceFilter, presenceFilter, phaseInverterBandwidth;
    oap::dsp::PowerAmp powerAmp;
    oap::dsp::CabinetSection cabinet;

    juce::SmoothedValue<float> tight, gain, bass, middle, treble, presence, resonance, master, boostMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;

    // values derived once per chunk
    double interstageGain = 1.0, powerDrive = 1.0, powerMakeup = 1.0, boostAmount = 0.0;
    float appliedTight = -1.0f, appliedBass = -1.0f, appliedMiddle = -1.0f, appliedTreble = -1.0f,
          appliedPresence = -1.0f, appliedResonance = -1.0f;
};
