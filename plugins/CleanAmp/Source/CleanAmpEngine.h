#pragma once

#include <juce_dsp/juce_dsp.h>

#include "dsp/CabinetSection.h"
#include "dsp/OapDsp.h"

//==============================================================================
/** The complete Clean Amp signal path, independent of the plugin wrapper.

    bright cap ->
    ( 4x oversampled: V1 -> blackface tone stack -> volume -> V2 -> power amp with sag ) ->
    cabinet -> output -> stereo reverb

    The amp is mono like a real amp; the reverb spreads it to stereo when the
    output has two channels.
*/
class CleanAmpEngine
{
public:
    struct Params
    {
        float volume   = 4.5f;  // all knobs are 0..10
        bool  bright   = false;
        float bass     = 5.0f;
        float middle   = 5.0f;
        float treble   = 6.0f;
        float master   = 5.0f;
        float reverb   = 2.5f;
        float outputDb = 0.0f;
        int   cabinet  = oap::dsp::CabinetSection::Open2x12;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Processes the first numInputChannels as input and fills all channels of the buffer. */
    void process (juce::AudioBuffer<float>& buffer, int numInputChannels, const Params& params) noexcept;

    int getLatencySamples() const noexcept;
    oap::dsp::CabinetSection& getCabinet() noexcept { return cabinet; }

    static constexpr int oversamplingOrder = 2; // 2^2 = 4x

private:
    void setTargets (const Params&) noexcept;
    void advanceControls (int numSamples) noexcept;
    double processOversampled (double x) noexcept;
    void processReverb (juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

    static constexpr int chunkSize = 32;

    double sampleRate = 48000.0, osRate = 192000.0;
    int maxBlock = 512;
    bool snapControls = true;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    oap::dsp::Biquad brightFilter;
    oap::dsp::TubeStage stage1, stage2;
    oap::dsp::ToneStack toneStack;
    oap::dsp::PowerAmp powerAmp;
    oap::dsp::CabinetSection cabinet;

    juce::Reverb reverb;
    juce::AudioBuffer<float> reverbBuffer;
    oap::dsp::Biquad reverbHighpass[2], reverbLowpass[2];

    juce::SmoothedValue<float> volume, bass, middle, treble, master, brightMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain;
    float reverbAmount = 0.0f;

    double volumeGain = 1.0, powerDrive = 1.0, powerMakeup = 1.0;
    float appliedBright = -1.0f, appliedBass = -1.0f, appliedMiddle = -1.0f, appliedTreble = -1.0f;
};
