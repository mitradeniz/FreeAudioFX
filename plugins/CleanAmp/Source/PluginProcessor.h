#pragma once

#include "CleanAmpEngine.h"
#include "plugin/AmpProcessorBase.h"

class CleanAmpProcessor final : public oap::AmpProcessorBase
{
public:
    CleanAmpProcessor();

    const juce::String getName() const override { return JucePlugin_Name; }
    double getTailLengthSeconds() const override { return 3.0; } // reverb tail

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;

protected:
    oap::dsp::CabinetSection& getCabinet() noexcept override { return engine.getCabinet(); }

private:
    CleanAmpEngine engine;
    bool prepared = false;

    std::atomic<float>* volume;
    std::atomic<float>* bright;
    std::atomic<float>* bass;
    std::atomic<float>* middle;
    std::atomic<float>* treble;
    std::atomic<float>* reverb;
    std::atomic<float>* master;
    std::atomic<float>* output;
    std::atomic<float>* cabinet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanAmpProcessor)
};
