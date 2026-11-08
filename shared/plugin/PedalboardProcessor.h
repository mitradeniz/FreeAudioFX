#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "fx/Effect.h"
#include "fx/FxDsp.h"

#include <array>
#include <atomic>

namespace oap
{

//==============================================================================
/** A chain of eight pedal slots. Each slot can hold any effect from the
    library; its six knobs are generic parameters whose meaning (and display
    text) follows the effect currently loaded in the slot.

    Effects are created and destroyed on the message thread and handed to the
    audio thread through lock-free slots, so changing a pedal never blocks
    audio processing. */
class PedalboardProcessor : public juce::AudioProcessor,
                            private juce::Timer
{
public:
    static constexpr int numSlots = 8;
    static constexpr int knobsPerSlot = (int) fx::EffectInfo::maxParams;
    static constexpr const char* uiScaleProperty = "uiScale";

    PedalboardProcessor();
    ~PedalboardProcessor() override;

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    static juce::String typeId (int slot);
    static juce::String onId (int slot);
    static juce::String knobId (int slot, int knob);

    /** Effect selected for a slot (nullptr = empty). Safe from any thread. */
    const fx::EffectInfo* getSlotEffect (int slot) const noexcept;

    /** Message thread: loads an effect type into a slot and resets its knobs to that effect's defaults. */
    void setSlotEffect (int slot, int effectIndex);

    /** Message thread: swaps two slots (type, knobs and on/off). */
    void swapSlots (int a, int b);

    /** Index into fx::allEffects(), or -1 for an empty slot. */
    static int effectIndexForChoice (int choice) noexcept { return choice - 1; }

    //==============================================================================
    const juce::String getName() const override { return "OAP Pedalboard"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override { running = false; }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

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
    struct Slot
    {
        fx::Effect* current = nullptr;               // owned by the audio thread while running
        std::atomic<fx::Effect*> incoming { nullptr }; // message -> audio
        std::atomic<fx::Effect*> outgoing { nullptr }; // audio -> message (to delete)
        std::atomic<const fx::EffectInfo*> requested { nullptr }; // last effect created for this slot

        std::atomic<float>* type = nullptr;
        std::atomic<float>* on = nullptr;
        std::array<std::atomic<float>*, knobsPerSlot> knobs {};

        fx::DelayLine dryDelay[2];
        juce::SmoothedValue<float> wet;
        int latency = 0;
        bool fadeIn = false;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (PedalboardProcessor&);

    void timerCallback() override;
    void syncEffects (bool audioIsRunning);
    std::unique_ptr<fx::Effect> makePreparedEffect (const fx::EffectInfo&) const;
    void collectGarbage();
    void updateLatency();

    std::array<Slot, numSlots> slots;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* inputGain = nullptr;
    std::atomic<float>* outputGain = nullptr;
    juce::SmoothedValue<float> inGain, outGain;
    juce::AudioBuffer<float> dryBuffer;

    std::atomic<double> currentSampleRate { 0.0 };
    std::atomic<int> currentBlockSize { 512 };
    std::atomic<bool> running { false };
    juce::CriticalSection managementLock; // message-thread housekeeping only, never taken by the audio thread

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalboardProcessor)
};

} // namespace oap
