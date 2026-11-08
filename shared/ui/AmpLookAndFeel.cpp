#include "AmpLookAndFeel.h"

namespace oap::ui
{

using juce::Colour;

//==============================================================================
AmpTheme AmpTheme::drive()
{
    AmpTheme t;
    t.background   = Colour (0xff121214);
    t.panelTop     = Colour (0xff34363b);
    t.panelBottom  = Colour (0xff1f2023);
    t.panelEdge    = Colour (0xff0a0a0b);
    t.panelText    = Colour (0xffeceae6);
    t.panelTextDim = Colour (0xff8d9097);
    t.accent       = Colour (0xffff5a1f);
    t.knobBody     = Colour (0xff17171a);
    t.knobSkirt    = Colour (0xff60636b);
    t.knobPointer  = Colour (0xfff6f6f6);
    t.headerText   = Colour (0xfff2f2f2);
    t.readout      = Colour (0xff0b0b0c);
    return t;
}

AmpTheme AmpTheme::clean()
{
    AmpTheme t;
    t.background   = Colour (0xff2a231f);
    t.panelTop     = Colour (0xffeee9de);
    t.panelBottom  = Colour (0xffd2cbbb);
    t.panelEdge    = Colour (0xff8c8475);
    t.panelText    = Colour (0xff25221e);
    t.panelTextDim = Colour (0xff6e685d);
    t.accent       = Colour (0xff15a99a);
    t.knobBody     = Colour (0xff151515);
    t.knobSkirt    = Colour (0xffc3c7cc);
    t.knobPointer  = Colour (0xfff2f2f2);
    t.headerText   = Colour (0xfff1eadc);
    t.readout      = Colour (0xff1c1916);
    return t;
}

AmpTheme AmpTheme::pedal (Colour enclosure)
{
    const float brightness = enclosure.getPerceivedBrightness();
    const bool lightPaint = brightness > 0.6f;
    const bool veryDark = brightness < 0.22f;

    AmpTheme t;
    t.background   = Colour (0xff141518);
    t.panelTop     = enclosure.brighter (0.18f);
    t.panelBottom  = enclosure.darker (0.35f);
    t.panelEdge    = enclosure.darker (0.9f);
    t.panelText    = lightPaint ? Colour (0xff1c1c1c) : Colour (0xfff4f4f2);
    t.panelTextDim = t.panelText.withAlpha (0.7f);
    t.accent       = veryDark ? Colour (0xffff5a1f) : (lightPaint ? enclosure.darker (1.2f) : Colour (0xfff7f7f7));
    t.knobBody     = Colour (0xff141414);
    t.knobSkirt    = Colour (0xffc4c8cd);
    t.knobPointer  = Colour (0xfff4f4f4);
    t.headerText   = Colour (0xffe9e9e9);
    t.readout      = Colour (0xff111113);
    return t;
}

AmpTheme AmpTheme::custom (Colour background, Colour plateTop, Colour plateBottom, Colour accent, bool darkText)
{
    AmpTheme t;
    t.background   = background;
    t.panelTop     = plateTop;
    t.panelBottom  = plateBottom;
    t.panelEdge    = plateBottom.darker (0.8f);
    t.panelText    = darkText ? Colour (0xff201e1b) : Colour (0xffeeeeec);
    t.panelTextDim = darkText ? Colour (0xff6a655c) : Colour (0xff8f9298);
    t.accent       = accent;
    t.knobBody     = darkText ? Colour (0xff151515) : Colour (0xff17171a);
    t.knobSkirt    = darkText ? Colour (0xffc3c7cc) : Colour (0xff5f636b);
    t.knobPointer  = Colour (0xfff4f4f4);
    t.headerText   = Colour (0xfff0ede6);
    t.readout      = Colour (0xff0d0d0f);
    return t;
}

//==============================================================================
AmpLookAndFeel::AmpLookAndFeel (const AmpTheme& t) : theme (t)
{
    setColour (juce::Slider::textBoxTextColourId, theme.panelText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, theme.accent.withAlpha (0.4f));
    setColour (juce::Label::textColourId, theme.panelText);
    setColour (juce::Label::textWhenEditingColourId, theme.panelText);
    setColour (juce::TextEditor::textColourId, theme.panelText);
    setColour (juce::TextEditor::backgroundColourId, theme.panelBottom);
    setColour (juce::TextEditor::highlightColourId, theme.accent.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, theme.accent);

    setColour (juce::ComboBox::textColourId, theme.accent.brighter (0.4f));
    setColour (juce::ComboBox::backgroundColourId, theme.readout);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour (juce::ComboBox::arrowColourId, theme.accent);

    setColour (juce::PopupMenu::backgroundColourId, Colour (0xff1b1b1e));
    setColour (juce::PopupMenu::textColourId, Colour (0xffe6e6e6));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme.accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);

    setColour (juce::TextButton::buttonColourId, Colour (0xff2a2b2f));
    setColour (juce::TextButton::textColourOffId, Colour (0xffe6e6e6));
    setColour (juce::TextButton::textColourOnId, theme.accent);

    setColour (juce::AlertWindow::backgroundColourId, Colour (0xff1b1b1e));
    setColour (juce::AlertWindow::textColourId, Colour (0xffe6e6e6));
}

juce::Font AmpLookAndFeel::font (float height, bool bold)
{
    return juce::Font (juce::FontOptions().withHeight (height).withStyle (bold ? "Bold" : "Regular"));
}

//==============================================================================
void AmpLookAndFeel::paintCovering (juce::Graphics& g, juce::Rectangle<int> area, const AmpTheme& t)
{
    g.setColour (t.background);
    g.fillRect (area);

    // fine cross-hatch so the covering reads as a textured material
    juce::Random rng (1234);
    const auto r = area.toFloat();
    for (float d = -r.getHeight(); d < r.getWidth(); d += 3.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.012f + 0.012f * rng.nextFloat()));
        g.drawLine (r.getX() + d, r.getY(), r.getX() + d + r.getHeight(), r.getBottom(), 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.05f + 0.04f * rng.nextFloat()));
        g.drawLine (r.getX() + d + 1.5f, r.getBottom(), r.getX() + d + 1.5f + r.getHeight(), r.getY(), 1.0f);
    }

    // soft vignette
    juce::ColourGradient vignette (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                   juce::Colours::black.withAlpha (0.45f), r.getX(), r.getY(), true);
    g.setGradientFill (vignette);
    g.fillRect (area);
}

void AmpLookAndFeel::paintFaceplate (juce::Graphics& g, juce::Rectangle<float> area, const AmpTheme& t)
{
    // drop shadow
    juce::Path outline;
    outline.addRoundedRectangle (area, 7.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 18, { 0, 6 }).drawForPath (g, outline);

    g.setGradientFill (juce::ColourGradient (t.panelTop, area.getX(), area.getY(),
                                             t.panelBottom, area.getX(), area.getBottom(), false));
    g.fillPath (outline);

    // brushed-metal streaks
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (outline);
        juce::Random rng (77);
        for (float y = area.getY(); y < area.getBottom(); y += 1.0f)
        {
            const float a = 0.025f * rng.nextFloat();
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (a));
            g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
        }
    }

    // bevel
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawLine (area.getX() + 7.0f, area.getY() + 1.0f, area.getRight() - 7.0f, area.getY() + 1.0f, 1.0f);
    g.setColour (t.panelEdge);
    g.drawRoundedRectangle (area, 7.0f, 1.5f);

    // corner screws
    const float inset = 13.0f, sr = 4.5f;
    for (auto c : { area.getTopLeft().translated (inset, inset), area.getTopRight().translated (-inset, inset),
                    area.getBottomLeft().translated (inset, -inset), area.getBottomRight().translated (-inset, -inset) })
    {
        juce::Rectangle<float> screw (c.x - sr, c.y - sr, sr * 2, sr * 2);
        g.setGradientFill (juce::ColourGradient (Colour (0xffd8d8d8), screw.getX(), screw.getY(),
                                                 Colour (0xff6a6a6a), screw.getRight(), screw.getBottom(), false));
        g.fillEllipse (screw);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (screw, 0.8f);
        g.drawLine (c.x - sr * 0.6f, c.y + sr * 0.3f, c.x + sr * 0.6f, c.y - sr * 0.3f, 1.2f);
    }
}

void AmpLookAndFeel::paintGroupCaption (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text, const AmpTheme& t)
{
    const auto f = font (11.0f, true).withExtraKerningFactor (0.18f);
    g.setFont (f);
    const float textW = juce::GlyphArrangement::getStringWidth (f, text) + 14.0f;
    const float y = area.getCentreY();

    g.setColour (t.panelTextDim.withAlpha (0.55f));
    const float half = (area.getWidth() - textW) * 0.5f;
    if (half > 6.0f)
    {
        g.drawLine (area.getX() + 4.0f, y, area.getX() + half, y, 1.0f);
        g.drawLine (area.getRight() - half, y, area.getRight() - 4.0f, y, 1.0f);
    }

    g.setColour (t.panelTextDim);
    g.drawText (text, area, juce::Justification::centred, false);
}

//==============================================================================
void AmpLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                       float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float outer = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool active = slider.isMouseOverOrDragging() && slider.isEnabled();

    // scale ticks 0..10
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const float r1 = outer - (i % 5 == 0 ? 5.5f : 3.5f), r2 = outer;
        g.setColour (theme.panelTextDim.withAlpha (i % 5 == 0 ? 0.95f : 0.6f));
        g.drawLine (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a),
                    c.x + r2 * std::sin (a), c.y - r2 * std::cos (a), i % 5 == 0 ? 1.6f : 1.1f);
    }

    // value arc
    const float arcR = outer - 9.0f;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
    const juce::PathStrokeType stroke (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.strokePath (track, stroke);
    g.setColour (theme.accent.withAlpha (active ? 0.35f : 0.18f));
    g.strokePath (value, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (theme.accent);
    g.strokePath (value, stroke);

    // knob skirt (knurled metal)
    const float skirtR = arcR - 6.0f;
    juce::Path skirt;
    skirt.addEllipse (c.x - skirtR, c.y - skirtR, skirtR * 2, skirtR * 2);
    juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 10, { 0, 4 }).drawForPath (g, skirt);
    g.setGradientFill (juce::ColourGradient (theme.knobSkirt.brighter (0.35f), c.x, c.y - skirtR,
                                             theme.knobSkirt.darker (0.6f), c.x, c.y + skirtR, false));
    g.fillPath (skirt);

    g.setColour (juce::Colours::black.withAlpha (0.18f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = angle + (float) i * juce::MathConstants<float>::twoPi / 40.0f;
        g.drawLine (c.x + skirtR * 0.80f * std::sin (a), c.y - skirtR * 0.80f * std::cos (a),
                    c.x + skirtR * 0.98f * std::sin (a), c.y - skirtR * 0.98f * std::cos (a), 1.0f);
    }

    // cap
    const float capR = skirtR * 0.74f;
    g.setGradientFill (juce::ColourGradient (theme.knobBody.brighter (0.5f), c.x, c.y - capR,
                                             theme.knobBody.darker (0.4f), c.x, c.y + capR, false));
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.20f), c.x, c.y - capR,
                                             juce::Colours::transparentWhite, c.x, c.y, false));
    g.fillEllipse (c.x - capR * 0.8f, c.y - capR * 0.95f, capR * 1.6f, capR * 1.1f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2, 1.0f);

    // pointer
    const float p1 = capR * 0.25f, p2 = skirtR * 0.92f;
    juce::Line<float> pointer (c.x + p1 * std::sin (angle), c.y - p1 * std::cos (angle),
                               c.x + p2 * std::sin (angle), c.y - p2 * std::cos (angle));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawLine (pointer.withShortenedStart (-0.5f).withShortenedEnd (-0.5f), 4.5f);
    g.setColour (active ? theme.accent.brighter (0.3f) : theme.knobPointer);
    g.drawLine (pointer, 2.6f);
}

juce::Label* AmpLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (font (12.5f));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, theme.panelText.withAlpha (0.9f));
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

//==============================================================================
void AmpLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto area = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();

    // classic bat-handle toggle in a chrome bushing, with an indicator LED above
    const auto c = area.getCentre().translated (0.0f, 6.0f);
    const float bushingR = 11.0f;

    const juce::Rectangle<float> led (c.x - 4.0f, area.getY() + 2.0f, 8.0f, 8.0f);
    if (on)
    {
        g.setColour (theme.accent.withAlpha (0.35f));
        g.fillEllipse (led.expanded (4.0f));
    }
    g.setColour (on ? theme.accent.brighter (0.2f) : juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (led, 0.8f);

    juce::Path nut;
    nut.addEllipse (c.x - bushingR, c.y - bushingR, bushingR * 2, bushingR * 2);
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 6, { 0, 2 }).drawForPath (g, nut);
    g.setGradientFill (juce::ColourGradient (Colour (0xffe8e8e8), c.x, c.y - bushingR,
                                             Colour (0xff6c6c6c), c.x, c.y + bushingR, false));
    g.fillPath (nut);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (nut, juce::PathStrokeType (0.8f));

    const float leverLen = 20.0f;
    const float dir = on ? -1.0f : 1.0f;
    const juce::Point<float> tip (c.x, c.y + dir * leverLen);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine (c.x + 1.5f, c.y + 1.5f, tip.x + 1.5f, tip.y + 1.5f, 6.0f);
    g.setGradientFill (juce::ColourGradient (Colour (0xfff4f4f4), c.x - 3.0f, c.y,
                                             Colour (0xff8a8a8a), c.x + 3.0f, c.y, false));
    g.drawLine (c.x, c.y, tip.x, tip.y, 5.5f);
    g.fillEllipse (tip.x - 4.0f, tip.y - 4.0f, 8.0f, 8.0f);

    if (highlighted)
    {
        g.setColour (theme.accent.withAlpha (0.25f));
        g.drawEllipse (c.x - bushingR - 3.0f, c.y - bushingR - 3.0f, (bushingR + 3.0f) * 2, (bushingR + 3.0f) * 2, 1.5f);
    }
}

void AmpLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base,
                                           bool highlighted, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto col = base;
    if (down) col = col.darker (0.3f);
    else if (highlighted) col = col.brighter (0.15f);

    g.setGradientFill (juce::ColourGradient (col.brighter (0.15f), r.getX(), r.getY(),
                                             col.darker (0.25f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (highlighted ? theme.accent.withAlpha (0.8f) : juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

juce::Font AmpLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return font (juce::jmin (13.0f, (float) buttonHeight * 0.5f), true);
}

void AmpLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.isMouseOver (true) ? theme.accent.withAlpha (0.8f) : juce::Colours::black);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    // inner glow like a backlit display
    g.setColour (theme.accent.withAlpha (0.06f));
    g.fillRoundedRectangle (r.reduced (2.0f), 3.0f);

    juce::Path arrow;
    const float ax = (float) width - 16.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 5.0f, ay - 2.5f, ax + 5.0f, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

juce::Font AmpLookAndFeel::getComboBoxFont (juce::ComboBox&) { return font (14.0f, true); }

void AmpLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font AmpLookAndFeel::getPopupMenuFont() { return font (15.0f); }

juce::Font AmpLookAndFeel::getLabelFont (juce::Label& l)
{
    return l.getFont();
}

} // namespace oap::ui
