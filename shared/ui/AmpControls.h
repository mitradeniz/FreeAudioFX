#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "plugin/AmpProcessorBase.h"
#include "ui/AmpLookAndFeel.h"

namespace oap::ui
{

//==============================================================================
/** Rotary amp knob with its caption above and the value read-out below.
    Double-click resets to the default; drag with Shift held for fine control. */
class AmpKnob : public juce::Component
{
public:
    AmpKnob (juce::AudioProcessorValueTreeState&, const juce::String& parameterId, const juce::String& caption);

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    juce::String caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
/** Toggle switch with an indicator LED. */
class AmpSwitch : public juce::Component
{
public:
    AmpSwitch (juce::AudioProcessorValueTreeState&, const juce::String& parameterId, const juce::String& caption);

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::ToggleButton button;

private:
    juce::String caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

//==============================================================================
/** A labelled drop-down for choice parameters (modes, types, sync values). */
class AmpChoice : public juce::Component
{
public:
    AmpChoice (juce::AudioProcessorValueTreeState&, const juce::String& parameterId, const juce::String& caption);

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::ComboBox box;

private:
    juce::String caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

//==============================================================================
/** Cabinet selector plus impulse-response loader. */
class CabinetPanel : public juce::Component, private juce::Timer
{
public:
    explicit CabinetPanel (AmpProcessorBase&);
    ~CabinetPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void chooseImpulseResponse();
    void updateStatus();

    AmpProcessorBase& processor;
    juce::ComboBox cabinetBox;
    juce::TextButton loadButton { "Load IR..." }, clearButton { "Clear" };
    juce::Label status;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String lastError;
};

//==============================================================================
/** The amp "head": covering, logo, faceplate with grouped controls, and the
    cabinet strip. Plugins add their controls in groups and the panel lays
    them out. */
class AmpPanel : public juce::Component
{
public:
    struct Branding
    {
        juce::String modelAccent; // e.g. "DRIVE"
        juce::String modelRest;   // e.g. "AMP"
        juce::String tagline;     // e.g. "HIGH-GAIN AMPLIFIER"
    };

    AmpPanel (AmpProcessorBase&, const AmpTheme&, Branding, int width, int height);

    /** Adds a captioned group of controls (the components must outlive the panel's use of them). */
    void addGroup (const juce::String& caption, std::vector<juce::Component*> controls);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Group
    {
        juce::String caption;
        std::vector<juce::Component*> controls;
        juce::Rectangle<int> bounds;
    };

    struct OrnamentSpot
    {
        juce::Rectangle<float> area;
        bool round;
    };

    /** Everything that doesn't move (artwork, faceplate, logo, ornaments), drawn
        once into a cached image so turning a knob only redraws the knob. */
    struct Backdrop : juce::Component
    {
        explicit Backdrop (AmpPanel& p) : panel (p)
        {
            setInterceptsMouseClicks (false, false);
            setBufferedToImage (true);
        }

        void paint (juce::Graphics& g) override { panel.paintBackdrop (g); }

        AmpPanel& panel;
    };

    juce::Rectangle<int> getFaceplateArea() const;
    juce::Rectangle<int> getCabinetStripArea() const;
    void paintBackdrop (juce::Graphics&);
    void paintLogo (juce::Graphics&);

    AmpTheme theme;
    Branding branding;
    std::unique_ptr<juce::Drawable> art;
    int headerHeight = 80;
    std::vector<Group> groups;
    std::vector<OrnamentSpot> ornamentSpots;
    Backdrop backdrop { *this };
    CabinetPanel cabinetPanel;
};

//==============================================================================
/** Editor that hosts an AmpPanel at a fixed design size and scales it with the window. */
class AmpEditor : public juce::AudioProcessorEditor
{
public:
    AmpEditor (AmpProcessorBase&, const AmpTheme&);
    ~AmpEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

protected:
    /** Call once from the derived constructor. */
    void setPanel (std::unique_ptr<AmpPanel>);

    AmpLookAndFeel lookAndFeel;

private:
    AmpProcessorBase& ampProcessor;
    std::unique_ptr<AmpPanel> panel;
    int baseWidth = 900, baseHeight = 420;
};

} // namespace oap::ui
