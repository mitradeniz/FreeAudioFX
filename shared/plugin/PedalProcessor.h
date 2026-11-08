#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "fx/Effect.h"
#include "fx/FxDsp.h"

#include <array>

namespace oap
{

//==============================================================================
/** Generic plugin wrapper for one effect from the oap_fx library. Every pedal
    plugin is this class with a different effect id: the parameters, the
    editor and the state handling all come from the effect's description. */
class PedalProcessor : public juce::AudioProcessor
{
public:
    static constexpr const char* bypassId = "bypass";
    static constexpr const char* uiScaleProperty = "uiScale";

    explicit PedalProcessor (const std::string& effectId);
    ~PedalProcessor() override;

    const fx::EffectInfo& getEffectInfo() const noexcept { return info; }
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    //==============================================================================
    const juce::String getName() const override { return "OAP " + juce::String (info.name); }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return effect->getTailSeconds(); }

    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return {}; }
    void changeProgramName (int, const juce::String&) override  {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const fx::EffectInfo&);

    const fx::EffectInfo& info;
    std::unique_ptr<fx::Effect> effect;
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> values;
    std::atomic<float>* bypass = nullptr;

    std::array<fx::DelayLine, 2> dryDelay;
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> wetAmount;
    bool prepared = false, wasBypassed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalProcessor)
};

} // namespace oap
