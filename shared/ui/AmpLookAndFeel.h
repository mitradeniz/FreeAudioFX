#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace oap::ui
{

//==============================================================================
/** Decorations drawn around the knobs, switches and selectors of an amp, so
    the controls look like part of its artwork. */
enum class Ornament
{
    none,
    halo,       // soft glow
    neon,       // bright tube ring
    pixel,      // stepped 8-bit outline
    thorns,     // spiky ring
    ornate,     // double ring with beads
    stitch,     // dashed leather stitching
    petals,     // blossom petals
    star,       // eight-pointed star
    rays,       // sun rays
    bolts,      // lightning bolts
    pentagram,  // circle and inverted pentagram
    orbit,      // tilted orbit with a moon
    sparkles,   // disco sparkles
    rose        // stained-glass rose window
};

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

    // Artwork (optional): an SVG from shared/art drawn across the whole amp,
    // ornaments around the controls, and a see-through faceplate.
    juce::String art;
    Ornament ornament = Ornament::none;
    float plateOpacity = 1.0f;

    static AmpTheme drive();
    static AmpTheme clean();

    /** Theme for a stompbox enclosure of the given colour (text colours adapt to it). */
    static AmpTheme pedal (juce::Colour enclosure);

    /** Theme built from a few key colours (used by the effect amps). */
    static AmpTheme custom (juce::Colour background, juce::Colour plateTop, juce::Colour plateBottom,
                            juce::Colour accent, bool darkText);
};

//==============================================================================
class AmpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit AmpLookAndFeel (const AmpTheme& theme);

    const AmpTheme& getTheme() const noexcept { return theme; }

    static juce::Font font (float height, bool bold = false);

    /** Brushed faceplate with a bevel and corner screws (see-through when the theme has artwork). */
    static void paintFaceplate (juce::Graphics&, juce::Rectangle<float> area, const AmpTheme&);

    /** Radius of a knob's scale inside `bounds` (smaller when the theme draws ornaments, to leave room for them). */
    static float dialRadius (juce::Rectangle<float> bounds, const AmpTheme&);

    /** Draws the theme's ornament around a control. `area` is the circle just outside a knob's scale
        (ornaments stay within 7 px of it), or the switch/selector bounds. */
    static void paintOrnament (juce::Graphics&, juce::Rectangle<float> area, bool round, const AmpTheme&);

    /** Loads one of the embedded artworks (shared/art/<name>.svg), or returns nullptr. */
    static std::unique_ptr<juce::Drawable> loadArt (const juce::String& name);

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
