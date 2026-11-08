#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace oap::ui
{

//==============================================================================
/** Colour scheme for one amp. Every plugin picks its own so they are easy to
    tell apart in a DAW, while sharing the same drawing code. */
struct AmpTheme
{
    juce::Colour background;   // cabinet covering around the faceplate
    juce::Colour panelTop;     // faceplate gradient
    juce::Colour panelBottom;
    juce::Colour panelEdge;
    juce::Colour panelText;    // control labels printed on the faceplate
    juce::Colour panelTextDim;
    juce::Colour accent;       // value arcs, LEDs, logo highlight
    juce::Colour knobBody;
    juce::Colour knobSkirt;
    juce::Colour knobPointer;
    juce::Colour headerText;   // logo on the cabinet covering
    juce::Colour readout;      // value read-out background

    static AmpTheme drive();
    static AmpTheme clean();
};

//==============================================================================
class AmpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit AmpLookAndFeel (const AmpTheme& theme);

    const AmpTheme& getTheme() const noexcept { return theme; }

    static juce::Font font (float height, bool bold = false);

    /** Brushed faceplate with a bevel and corner screws. */
    static void paintFaceplate (juce::Graphics&, juce::Rectangle<float> area, const AmpTheme&);

    /** Textured covering (tolex) used as the editor background. */
    static void paintCovering (juce::Graphics&, juce::Rectangle<int> area, const AmpTheme&);

    /** Thin engraved divider with a small group caption. */
    static void paintGroupCaption (juce::Graphics&, juce::Rectangle<float> area, const juce::String& text, const AmpTheme&);

    //==============================================================================
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    juce::Font getPopupMenuFont() override;
    juce::Font getLabelFont (juce::Label&) override;

private:
    AmpTheme theme;
};

} // namespace oap::ui
