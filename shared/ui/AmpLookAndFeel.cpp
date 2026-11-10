#include "AmpLookAndFeel.h"

#include "OapArt.h"

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
    t.art          = "drive";
    t.ornament     = Ornament::thorns;
    t.plateOpacity = 0.78f;
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
    t.art          = "clean";
    t.ornament     = Ornament::sparkles;
    t.plateOpacity = 0.80f;
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
    t.panelTextDim = darkText ? Colour (0xff4c473f) : Colour (0xff8f9298);
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

    // Combo boxes are drawn as dark read-outs, so their text must stay light.
    const auto comboText = theme.accent.getPerceivedBrightness() < 0.45f ? Colour (0xffeeeeee) : theme.accent.brighter (0.4f);
    setColour (juce::ComboBox::textColourId, comboText);
    setColour (juce::ComboBox::backgroundColourId, theme.readout);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour (juce::ComboBox::arrowColourId, comboText);

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
    const float alpha = juce::jlimit (0.0f, 1.0f, t.plateOpacity);

    // drop shadow
    juce::Path outline;
    outline.addRoundedRectangle (area, 7.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f * alpha), 18, { 0, 6 }).drawForPath (g, outline);

    g.setGradientFill (juce::ColourGradient (t.panelTop.withMultipliedAlpha (alpha), area.getX(), area.getY(),
                                             t.panelBottom.withMultipliedAlpha (alpha), area.getX(), area.getBottom(), false));
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
std::unique_ptr<juce::Drawable> AmpLookAndFeel::loadArt (const juce::String& name)
{
    if (name.isEmpty())
        return nullptr;

    int size = 0;
    const auto resource = (name + "_svg").replaceCharacter ('-', '_');
    if (const char* data = OapArt::getNamedResource (resource.toRawUTF8(), size))
        if (auto drawable = juce::Drawable::createFromImageData (data, (size_t) size))
            return drawable;

    jassertfalse; // missing or broken artwork
    return nullptr;
}

namespace
{
    using juce::Point;
    using juce::Path;
    using juce::PathStrokeType;

    Point<float> polar (Point<float> c, float r, float angle) // angle 0 = up, clockwise
    {
        return { c.x + r * std::sin (angle), c.y - r * std::cos (angle) };
    }

    void strokeCircle (juce::Graphics& g, Point<float> c, float r, float width)
    {
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, width);
    }

    Path starPolygon (Point<float> c, int points, float outer, float inner, float rotation)
    {
        Path p;
        for (int i = 0; i < points * 2; ++i)
        {
            const float a = rotation + (float) i * juce::MathConstants<float>::pi / (float) points;
            const auto pt = polar (c, (i % 2 == 0) ? outer : inner, a);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        return p;
    }

    Path bolt (Point<float> c, float h, bool mirrored)
    {
        const float s = mirrored ? -1.0f : 1.0f, w = h * 0.45f;
        Path p;
        p.startNewSubPath (c.x + s * 0.15f * w, c.y - 0.5f * h);
        p.lineTo (c.x - s * 0.45f * w, c.y + 0.08f * h);
        p.lineTo (c.x + s * 0.02f * w, c.y + 0.05f * h);
        p.lineTo (c.x - s * 0.20f * w, c.y + 0.5f * h);
        p.lineTo (c.x + s * 0.50f * w, c.y - 0.12f * h);
        p.lineTo (c.x + s * 0.05f * w, c.y - 0.08f * h);
        p.lineTo (c.x + s * 0.35f * w, c.y - 0.5f * h);
        p.closeSubPath();
        return p;
    }

    void petal (juce::Graphics& g, Point<float> c, float r, float angle, float len, juce::Colour colour)
    {
        Path p;
        p.addEllipse (-len * 0.32f, -len * 0.5f, len * 0.64f, len);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (polar (c, r, angle)));
        g.setColour (colour);
        g.fillPath (p);
        g.setColour (colour.darker (0.4f).withAlpha (0.8f));
        g.strokePath (p, PathStrokeType (0.6f));
    }

    void paintRoundOrnament (juce::Graphics& g, Point<float> c, float r, const oap::ui::AmpTheme& t)
    {
        // r is just outside the knob's scale; every style stays within r + 7.
        using oap::ui::Ornament;
        const auto a = t.accent;
        const auto ink = t.panelText;
        const float twoPi = juce::MathConstants<float>::twoPi;

        switch (t.ornament)
        {
            case Ornament::none:
                break;

            case Ornament::halo:
            {
                const float outer = r + 7.0f;
                juce::ColourGradient glow (a.withAlpha (0.0f), c.x, c.y, a.withAlpha (0.0f), c.x + outer, c.y, true);
                glow.addColour ((r - 7.0f) / outer, a.withAlpha (0.0f));
                glow.addColour (r / outer, a.withAlpha (0.45f));
                g.setGradientFill (glow);
                g.fillEllipse (c.x - outer, c.y - outer, outer * 2.0f, outer * 2.0f);
                g.setColour (a.withAlpha (0.6f));
                strokeCircle (g, c, r + 0.5f, 0.8f);
                break;
            }

            case Ornament::neon:
                g.setColour (a.withAlpha (0.18f)); strokeCircle (g, c, r + 3.0f, 7.0f);
                g.setColour (a.withAlpha (0.40f)); strokeCircle (g, c, r + 3.0f, 3.5f);
                g.setColour (a.brighter (0.7f));   strokeCircle (g, c, r + 3.0f, 1.3f);
                break;

            case Ornament::pixel:
            {
                const float cell = 3.0f;
                juce::Array<juce::Point<int>> done;
                for (int i = 0; i < 220; ++i)
                {
                    const auto pt = polar (c, r + 3.0f, twoPi * (float) i / 220.0f);
                    const juce::Point<int> cellPos ((int) std::floor (pt.x / cell), (int) std::floor (pt.y / cell));
                    if (done.addIfNotAlreadyThere (cellPos))
                    {
                        g.setColour ((cellPos.x + cellPos.y) % 2 == 0 ? a : a.withAlpha (0.55f));
                        g.fillRect ((float) cellPos.x * cell, (float) cellPos.y * cell, cell - 0.4f, cell - 0.4f);
                    }
                }
                for (int i = 0; i < 8; ++i) // little "power-up" pixels on the diagonals
                {
                    const auto pt = polar (c, r + 6.0f, twoPi * ((float) i + 0.5f) / 8.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.85f));
                    g.fillRect (std::floor (pt.x) - 1.5f, std::floor (pt.y) - 1.5f, 3.0f, 3.0f);
                }
                break;
            }

            case Ornament::thorns:
            {
                Path spikes;
                const int n = 20;
                for (int i = 0; i < n; ++i)
                {
                    const float ang = twoPi * (float) i / (float) n;
                    const float len = (i % 2 == 0) ? 7.0f : 4.0f;
                    spikes.addTriangle (polar (c, r, ang - 0.08f), polar (c, r + len, ang), polar (c, r, ang + 0.08f));
                }
                g.setColour (a.withAlpha (0.85f));
                g.fillPath (spikes);
                strokeCircle (g, c, r + 0.5f, 1.4f);
                break;
            }

            case Ornament::ornate:
            {
                g.setColour (a.withAlpha (0.85f));
                strokeCircle (g, c, r + 0.5f, 1.1f);
                g.setColour (a.withAlpha (0.55f));
                strokeCircle (g, c, r + 6.5f, 0.9f);
                for (int i = 0; i < 24; ++i)
                {
                    const auto p = polar (c, r + 3.5f, twoPi * (float) i / 24.0f);
                    const bool big = i % 3 == 0;
                    const float br = big ? 1.7f : 0.9f;
                    g.setColour (big ? a : a.withAlpha (0.7f));
                    g.fillEllipse (p.x - br, p.y - br, br * 2.0f, br * 2.0f);
                }
                break;
            }

            case Ornament::stitch:
            {
                Path circle, dashed;
                circle.addEllipse (c.x - r - 3.0f, c.y - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f);
                const float dashes[] = { 4.0f, 3.5f };
                PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded).createDashedStroke (dashed, circle, dashes, 2);
                g.setColour (ink.withAlpha (0.65f));
                g.fillPath (dashed);
                g.setColour (a.withAlpha (0.5f));
                strokeCircle (g, c, r + 6.5f, 1.0f);
                strokeCircle (g, c, r - 0.5f, 1.0f);
                break;
            }

            case Ornament::petals:
            {
                const juce::Colour pink (0xfff6b3c6);
                for (int i = 0; i < 5; ++i)
                    petal (g, c, r + 3.5f, twoPi * (float) i / 5.0f + 0.31f, 7.5f, pink.withAlpha (0.95f));
                for (int i = 0; i < 5; ++i)
                {
                    const auto p = polar (c, r + 2.5f, twoPi * (float) i / 5.0f + 0.31f + 0.63f);
                    g.setColour (a.withAlpha (0.8f));
                    g.fillEllipse (p.x - 1.3f, p.y - 1.3f, 2.6f, 2.6f);
                }
                break;
            }

            case Ornament::star:
            {
                const float s = r + 7.0f;
                Path star;
                for (float rot : { 0.0f, juce::MathConstants<float>::pi * 0.25f })
                {
                    Path sq;
                    sq.addRectangle (-s * 0.7071f, -s * 0.7071f, s * 1.4142f, s * 1.4142f);
                    sq.applyTransform (juce::AffineTransform::rotation (rot).translated (c));
                    star.addPath (sq);
                }
                g.setColour (a.withAlpha (0.16f));
                g.fillPath (star);
                g.setColour (a.withAlpha (0.8f));
                g.strokePath (star, PathStrokeType (1.1f));
                strokeCircle (g, c, r + 0.5f, 0.9f);
                break;
            }

            case Ornament::rays:
            {
                g.setColour (a.withAlpha (0.9f));
                for (int i = 0; i < 32; ++i)
                {
                    const float ang = twoPi * (float) i / 32.0f;
                    const float len = (i % 2 == 0) ? 6.5f : 3.5f;
                    g.drawLine (juce::Line<float> (polar (c, r + 0.5f, ang), polar (c, r + 0.5f + len, ang)), i % 2 == 0 ? 1.5f : 1.0f);
                }
                strokeCircle (g, c, r, 1.3f);
                break;
            }

            case Ornament::bolts:
            {
                g.setColour (a.withAlpha (0.25f)); strokeCircle (g, c, r + 1.5f, 5.0f);
                g.setColour (a);                   strokeCircle (g, c, r + 1.5f, 1.2f);
                for (float ang : { -2.2f, 2.2f, -1.05f, 1.05f })
                    g.fillPath (bolt (polar (c, r + 3.5f, ang), 10.0f, ang > 0.0f));
                break;
            }

            case Ornament::pentagram:
            {
                const float outer = r + 4.5f;
                Path star;
                for (int i = 0; i < 5; ++i)
                {
                    // inverted: first point at the bottom
                    const auto p = polar (c, outer, juce::MathConstants<float>::pi + twoPi * (float) (i * 2 % 5) / 5.0f);
                    if (i == 0) star.startNewSubPath (p); else star.lineTo (p);
                }
                star.closeSubPath();
                g.setColour (a.withAlpha (0.6f));
                g.strokePath (star, PathStrokeType (1.2f));
                g.setColour (a.withAlpha (0.85f));
                strokeCircle (g, c, outer, 1.3f);
                g.setColour (a.withAlpha (0.4f));
                strokeCircle (g, c, outer + 2.5f, 0.8f);
                for (int i = 0; i < 5; ++i)
                {
                    const auto p = polar (c, outer + 2.5f, juce::MathConstants<float>::pi + twoPi * (float) i / 5.0f);
                    g.setColour (a);
                    g.fillEllipse (p.x - 1.6f, p.y - 1.6f, 3.2f, 3.2f);
                }
                break;
            }

            case Ornament::orbit:
            {
                const float rx = r + 6.0f;
                Path orbitPath;
                orbitPath.addEllipse (-rx, -rx * 0.38f, rx * 2.0f, rx * 0.76f);
                orbitPath.applyTransform (juce::AffineTransform::rotation (-0.35f).translated (c));
                g.setColour (a.withAlpha (0.75f));
                g.strokePath (orbitPath, PathStrokeType (1.3f));
                g.setColour (a.withAlpha (0.35f));
                strokeCircle (g, c, r + 0.5f, 1.0f);
                const auto moon = juce::Point<float> (-rx * 0.82f, -rx * 0.38f * 0.57f)
                                      .transformedBy (juce::AffineTransform::rotation (-0.35f).translated (c));
                g.setColour (a.brighter (0.5f));
                g.fillEllipse (moon.x - 3.0f, moon.y - 3.0f, 6.0f, 6.0f);
                break;
            }

            case Ornament::sparkles:
            {
                for (int i = 0; i < 16; ++i)
                {
                    const auto p = polar (c, r + 3.0f, twoPi * (float) i / 16.0f);
                    if (i % 4 == 0)
                    {
                        g.setColour (juce::Colours::white.withAlpha (0.95f));
                        g.fillPath (starPolygon (p, 4, 4.5f, 1.1f, 0.0f));
                    }
                    else
                    {
                        g.setColour ((i % 2 == 0 ? a : juce::Colour (0xffd8f3ff)).withAlpha (0.85f));
                        g.fillEllipse (p.x - 1.4f, p.y - 1.4f, 2.8f, 2.8f);
                    }
                }
                break;
            }

            case Ornament::rose:
            {
                static const juce::Colour glass[] = { juce::Colour (0xffc0283b), juce::Colour (0xff2a55c8), juce::Colour (0xffe0a82e),
                                                      juce::Colour (0xff2f8f5a), juce::Colour (0xff7b3fb5), juce::Colour (0xff2a55c8) };
                const int n = 18;
                const float outer = r + 6.5f;
                for (int i = 0; i < n; ++i)
                {
                    const float a0 = twoPi * (float) i / (float) n, a1 = twoPi * (float) (i + 1) / (float) n;
                    Path seg;
                    seg.addPieSegment (c.x - outer, c.y - outer, outer * 2.0f, outer * 2.0f, a0, a1, (r + 0.5f) / outer);
                    g.setColour (glass[i % 6].withAlpha (0.85f));
                    g.fillPath (seg);
                }
                g.setColour (juce::Colour (0xff1c1a20));
                for (int i = 0; i < n; ++i)
                {
                    const float ang = twoPi * (float) i / (float) n;
                    g.drawLine (juce::Line<float> (polar (c, r + 0.5f, ang), polar (c, outer, ang)), 1.1f);
                }
                strokeCircle (g, c, r + 0.5f, 1.3f);
                strokeCircle (g, c, outer, 1.5f);
                break;
            }
        }
    }

    void paintFrameOrnament (juce::Graphics& g, juce::Rectangle<float> area, const oap::ui::AmpTheme& t)
    {
        using oap::ui::Ornament;
        const auto a = t.accent;
        const auto r = area.expanded (4.0f, 4.0f);

        if (t.ornament == Ornament::none)
            return;

        if (t.ornament == Ornament::halo || t.ornament == Ornament::neon)
        {
            g.setColour (a.withAlpha (0.15f)); g.drawRoundedRectangle (r, 7.0f, 6.0f);
            g.setColour (a.withAlpha (0.35f)); g.drawRoundedRectangle (r, 7.0f, 3.0f);
            g.setColour (t.ornament == Ornament::neon ? a.brighter (0.7f) : a.withAlpha (0.6f));
            g.drawRoundedRectangle (r, 7.0f, 1.2f);
            return;
        }

        if (t.ornament == Ornament::pixel)
        {
            g.setColour (a);
            for (float x = r.getX(); x < r.getRight(); x += 5.0f)
            {
                g.fillRect (x, r.getY(), 3.0f, 3.0f);
                g.fillRect (x, r.getBottom() - 3.0f, 3.0f, 3.0f);
            }
            for (float y = r.getY() + 5.0f; y < r.getBottom() - 3.0f; y += 5.0f)
            {
                g.fillRect (r.getX(), y, 3.0f, 3.0f);
                g.fillRect (r.getRight() - 3.0f, y, 3.0f, 3.0f);
            }
            return;
        }

        if (t.ornament == Ornament::stitch)
        {
            Path p, dashed;
            p.addRoundedRectangle (r, 6.0f);
            const float dashes[] = { 4.0f, 3.5f };
            PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded).createDashedStroke (dashed, p, dashes, 2);
            g.setColour (t.panelText.withAlpha (0.6f));
            g.fillPath (dashed);
            return;
        }

        // every other style: a framed plate with corner jewels in the theme's motif
        g.setColour (a.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 5.0f, 1.2f);
        g.setColour (a.withAlpha (0.35f));
        g.drawRoundedRectangle (r.expanded (2.5f), 7.0f, 0.8f);
        for (auto corner : { r.getTopLeft(), r.getTopRight(), r.getBottomLeft(), r.getBottomRight() })
        {
            g.setColour (a);
            g.fillPath (starPolygon (corner, 4, 4.0f, 1.3f, 0.0f));
        }
    }
}

float AmpLookAndFeel::dialRadius (juce::Rectangle<float> bounds, const AmpTheme& t)
{
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    return t.ornament == Ornament::none ? r : juce::jmin (r, 34.0f);
}

void AmpLookAndFeel::paintOrnament (juce::Graphics& g, juce::Rectangle<float> area, bool round, const AmpTheme& t)
{
    if (t.ornament == Ornament::none)
        return;

    if (round)
        paintRoundOrnament (g, area.getCentre(), area.getWidth() * 0.5f, t);
    else
        paintFrameOrnament (g, area, t);
}

//==============================================================================
void AmpLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                       float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float outer = dialRadius (bounds, theme);
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

juce::Font AmpLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return font (box.getWidth() < 100 ? 12.5f : 14.0f, true); // narrow selectors on the amp faceplates
}

void AmpLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    const int arrow = box.getWidth() < 100 ? 22 : 30;
    label.setBounds (box.getWidth() < 100 ? 5 : 8, 1, box.getWidth() - arrow - (box.getWidth() < 100 ? 2 : 0), box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font AmpLookAndFeel::getPopupMenuFont() { return font (15.0f); }

juce::Font AmpLookAndFeel::getLabelFont (juce::Label& l)
{
    return l.getFont();
}

} // namespace oap::ui
