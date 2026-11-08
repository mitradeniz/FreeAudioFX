#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "plugin/PedalboardProcessor.h"
#include "ui/AmpControls.h"
#include "ui/AmpLookAndFeel.h"
#include "ui/PedalEditor.h"

namespace oap::ui
{

//==============================================================================
/** One slot of the board: effect picker, six context-sensitive knobs,
    footswitch and buttons to move the pedal along the chain. */
class PedalSlot : public juce::Component
{
public:
    PedalSlot (PedalboardProcessor&, int slotIndex);
    ~PedalSlot() override;

    /** Re-reads the slot's effect type (called regularly by the editor). */
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct MiniKnob : juce::Component
    {
        MiniKnob (juce::AudioProcessorValueTreeState&, const juce::String& parameterId);
        void paint (juce::Graphics&) override;
        void resized() override;

        juce::Slider slider;
        juce::String caption;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void effectChosen();

    PedalboardProcessor& board;
    const int index;
    const fx::EffectInfo* shownEffect = nullptr;
    bool initialised = false;

    std::unique_ptr<AmpLookAndFeel> lookAndFeel;
    juce::ComboBox picker;
    juce::TextButton moveLeft { "<" }, moveRight { ">" };
    juce::OwnedArray<MiniKnob> knobs;
    Footswitch footswitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footswitchAttachment;
    juce::Rectangle<float> ledArea;
};

//==============================================================================
class PedalboardEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit PedalboardEditor (PedalboardProcessor&);
    ~PedalboardEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int baseWidth = 1300, baseHeight = 620;

private:
    struct Content : juce::Component
    {
        void paint (juce::Graphics&) override;
        AmpTheme theme;
    };

    void timerCallback() override;

    PedalboardProcessor& board;
    AmpLookAndFeel lookAndFeel;
    Content content;
    juce::OwnedArray<PedalSlot> slots;
    AmpKnob inputKnob, outputKnob;
};

} // namespace oap::ui
