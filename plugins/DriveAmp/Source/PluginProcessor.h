#pragma once

#include "DriveAmpEngine.h"
#include "plugin/AmpProcessorBase.h"

class DriveAmpProcessor final : public oap::AmpProcessorBase
{
public:
    DriveAmpProcessor();

    const juce::String getName() const override { return JucePlugin_Name; }
    double getTailLengthSeconds() const override { return 0.1; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;

protected:
    oap::dsp::CabinetSection& getCabinet() noexcept override { return engine.getCabinet(); }

private:
    DriveAmpEngine engine;
    bool prepared = false;

    std::atomic<float>* gate;
    std::atomic<float>* tight;
    std::atomic<float>* boost;
    std::atomic<float>* gain;
    std::atomic<float>* bass;
    std::atomic<float>* middle;
    std::atomic<float>* treble;
    std::atomic<float>* presence;
    std::atomic<float>* resonance;
    std::atomic<float>* master;
    std::atomic<float>* output;
    std::atomic<float>* cabinet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriveAmpProcessor)
};
