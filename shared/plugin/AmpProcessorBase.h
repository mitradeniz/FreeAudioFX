#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/CabinetSection.h"

namespace oap
{

//==============================================================================
/** Shared AudioProcessor plumbing for the amp plugins:
    bus layouts, parameter state, preset/session save & restore, and the user
    impulse response (whose file path is stored with the session).
*/
class AmpProcessorBase : public juce::AudioProcessor
{
public:
    static constexpr const char* irFileProperty = "irFile";
    static constexpr const char* uiScaleProperty = "uiScale";

    AmpProcessorBase (juce::AudioProcessorValueTreeState::ParameterLayout layout, juce::String cabinetParameterId);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }
    const juce::String& getCabinetParameterId() const noexcept { return cabinetParamId; }

    //==============================================================================
    /** Message thread. Loads and selects an impulse response. Returns an error message, or empty on success. */
    juce::String loadImpulseResponse (const juce::File& file);

    /** Message thread. Forgets the user IR. */
    void clearImpulseResponse();

    juce::File getImpulseResponseFile() const;
    bool isImpulseResponseLoaded() noexcept { return getCabinet().hasUserIR(); }
    int getActiveCabinet() noexcept { return getCabinet().getActiveType(); }

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool hasEditor() const override    { return true; }

    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return {}; }
    void changeProgramName (int, const juce::String&) override  {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Delays the dry signal by the amp's latency so bypassing doesn't shift the track in time. */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlockBypassed;

protected:
    /** Call from prepareToPlay once the engine's latency is known. */
    void prepareBypass (double sampleRate, int maxBlockSize, int latencyInSamples);

    virtual dsp::CabinetSection& getCabinet() noexcept = 0;

    /** Cached pointer to a parameter's current value, for lock-free reads on the audio thread. */
    std::atomic<float>* rawParameter (const char* id) const;

    juce::AudioProcessorValueTreeState parameters;

private:
    juce::String cabinetParamId;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> bypassDelay { 4096 };
    bool bypassPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpProcessorBase)
};

//==============================================================================
/** Helpers for building amp-style parameter layouts. */
namespace params
{
    /** A 0..10 amp knob that displays like "6.5". */
    std::unique_ptr<juce::AudioParameterFloat> knob (const char* id, const juce::String& name, float defaultValue);

    /** Output level in dB. */
    std::unique_ptr<juce::AudioParameterFloat> outputLevel (const char* id, float minDb, float maxDb);

    /** Gate threshold in dB with "Off" at the bottom of the range. */
    std::unique_ptr<juce::AudioParameterFloat> gateThreshold (const char* id, float defaultDb);

    std::unique_ptr<juce::AudioParameterBool> toggle (const char* id, const juce::String& name, bool defaultValue);

    std::unique_ptr<juce::AudioParameterChoice> cabinetChoice (const char* id, int defaultIndex);
}

} // namespace oap
