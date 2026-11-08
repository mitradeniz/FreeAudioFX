#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "plugin/PedalProcessor.h"
#include "ui/AmpControls.h"
#include "ui/AmpLookAndFeel.h"

namespace oap::ui
{

//==============================================================================
/** Chrome stomp switch. Toggle state = bypassed. */
class Footswitch : public juce::Button
{
public:
    Footswitch();
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

//==============================================================================
/** Draws a stompbox enclosure and lays out an effect's controls on it.
    Used by the single-pedal plugins and, scaled down, by the Pedalboard. */
class PedalFace : public juce::Component
{
public:
    PedalFace (const fx::EffectInfo&, juce::AudioProcessorValueTreeState&, const juce::String& idPrefix,
               const juce::String& bypassParameterId);

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int defaultWidth = 332, defaultHeight = 600;

private:
    const fx::EffectInfo& info;
    juce::OwnedArray<juce::Component> knobs, selectors;
    Footswitch footswitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footswitchAttachment;
    juce::Rectangle<float> ledArea;
    juce::Rectangle<int> nameArea;
};

//==============================================================================
class PedalEditor : public juce::AudioProcessorEditor
{
public:
    explicit PedalEditor (PedalProcessor&);
    ~PedalEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int baseWidth = PedalFace::defaultWidth + 48;
    static constexpr int baseHeight = PedalFace::defaultHeight + 48;

    PedalProcessor& pedal;
    AmpLookAndFeel lookAndFeel;
    juce::Component content;
    PedalFace face;
};

} // namespace oap::ui
