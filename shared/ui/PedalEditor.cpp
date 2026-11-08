#include "PedalEditor.h"

namespace oap::ui
{

namespace
{
    const AmpTheme& themeOf (const juce::Component& c)
    {
        if (auto* lnf = dynamic_cast<AmpLookAndFeel*> (&c.getLookAndFeel()))
            return lnf->getTheme();
        static const AmpTheme fallback = AmpTheme::drive();
        return fallback;
    }

    void layoutRow (juce::Array<juce::Component*> items, juce::Rectangle<int> row, int cellWidth)
    {
        const int total = cellWidth * items.size();
        int x = row.getCentreX() - total / 2;
        for (auto* c : items)
        {
            c->setBounds (x, row.getY(), cellWidth, row.getHeight());
            x += cellWidth;
        }
    }
}

//==============================================================================
Footswitch::Footswitch() : juce::Button ("Footswitch")
{
    setClickingTogglesState (true);
    setTitle ("Bypass");
    setTooltip ("Switch the effect on or off");
}

void Footswitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto r = getLocalBounds().toFloat().reduced (4.0f);
    const auto c = r.getCentre();
    const float outer = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;

    // hex nut
    juce::Path nut;
    for (int i = 0; i < 6; ++i)
    {
        const float a = juce::MathConstants<float>::pi / 3.0f * (float) i + juce::MathConstants<float>::pi / 6.0f;
        const juce::Point<float> pt (c.x + outer * std::cos (a), c.y + outer * std::sin (a));
        if (i == 0) nut.startNewSubPath (pt); else nut.lineTo (pt);
    }
    nut.closeSubPath();
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 10, { 0, 4 }).drawForPath (g, nut);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe0e0e0), c.x, c.y - outer,
                                             juce::Colour (0xff6b6b6b), c.x, c.y + outer, false));
    g.fillPath (nut);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (nut, juce::PathStrokeType (1.0f));

    // plunger
    const float capR = outer * (down ? 0.56f : 0.62f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffafafa), c.x - capR * 0.4f, c.y - capR,
                                             juce::Colour (0xff8a8a8a), c.x + capR * 0.5f, c.y + capR, true));
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawEllipse (c.x - capR * 0.55f, c.y - capR * 0.55f, capR * 1.1f, capR * 1.1f, 1.0f);

    if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
    }
}

//==============================================================================
PedalFace::PedalFace (const fx::EffectInfo& infoIn, juce::AudioProcessorValueTreeState& state,
                      const juce::String& idPrefix, const juce::String& bypassParameterId)
    : info (infoIn)
{
    for (const auto& spec : info.params)
    {
        const auto id = idPrefix + juce::String (spec.id);
        switch (spec.type)
        {
            case fx::ParamType::Continuous:
                knobs.add (new AmpKnob (state, id, spec.name));
                break;
            case fx::ParamType::Choice:
                selectors.add (new AmpChoice (state, id, spec.name));
                break;
            case fx::ParamType::Toggle:
                selectors.add (new AmpSwitch (state, id, spec.name));
                break;
        }
    }

    for (auto* k : knobs)     addAndMakeVisible (k);
    for (auto* s : selectors) addAndMakeVisible (s);

    addAndMakeVisible (footswitch);
    footswitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, bypassParameterId, footswitch);
    footswitch.onStateChange = [this] { repaint (ledArea.toNearestInt().expanded (8)); };

    setTitle (info.name);
    setDescription (info.description);
    setSize (defaultWidth, defaultHeight);
}

void PedalFace::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto body = getLocalBounds().toFloat().reduced (2.0f);

    juce::Path shape;
    shape.addRoundedRectangle (body, 18.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 22, { 0, 8 }).drawForPath (g, shape);

    g.setGradientFill (juce::ColourGradient (t.panelTop, body.getX(), body.getY(),
                                             t.panelBottom, body.getRight(), body.getBottom(), false));
    g.fillPath (shape);

    // hammertone-ish speckle
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (shape);
        juce::Random rng ((juce::int64) std::hash<std::string>() (info.id));
        for (int i = 0; i < 900; ++i)
        {
            const float x = body.getX() + rng.nextFloat() * body.getWidth();
            const float y = body.getY() + rng.nextFloat() * body.getHeight();
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.035f));
            g.fillEllipse (x, y, 2.5f, 2.5f);
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), body.getX(), body.getY(),
                                                 juce::Colours::transparentWhite, body.getX(), body.getY() + 90.0f, false));
        g.fillRect (body);
    }

    g.setColour (t.panelEdge.withAlpha (0.8f));
    g.strokePath (shape, juce::PathStrokeType (1.5f));

    // screws
    for (auto pt : { body.getTopLeft().translated (14, 14), body.getTopRight().translated (-14, 14),
                     body.getBottomLeft().translated (14, -14), body.getBottomRight().translated (-14, -14) })
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd6d6d6), pt.x - 4, pt.y - 4,
                                                 juce::Colour (0xff666666), pt.x + 4, pt.y + 4, false));
        g.fillEllipse (pt.x - 4.5f, pt.y - 4.5f, 9.0f, 9.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine (pt.x - 3.0f, pt.y, pt.x + 3.0f, pt.y, 1.2f);
    }

    // name plate
    auto name = nameArea;
    g.setColour (t.panelText);
    g.setFont (AmpLookAndFeel::font (info.name.size() > 12 ? 25.0f : 31.0f, true).withExtraKerningFactor (0.06f));
    g.drawFittedText (juce::String (info.name).toUpperCase(), name.removeFromTop (40), juce::Justification::centred, 1, 0.7f);
    g.setFont (AmpLookAndFeel::font (10.5f, true).withExtraKerningFactor (0.3f));
    g.setColour (t.panelTextDim);
    g.drawText ("OPEN AUDIO PLUGINS", name.removeFromTop (16), juce::Justification::centred, false);
    if (info.advanced)
    {
        g.setFont (AmpLookAndFeel::font (9.5f, true).withExtraKerningFactor (0.25f));
        g.drawText ("ADVANCED SERIES", name.removeFromTop (14), juce::Justification::centred, false);
    }

    // status LED
    const bool on = ! footswitch.getToggleState();
    const auto led = ledArea;
    if (on)
    {
        g.setColour (juce::Colour (0xffff2a2a).withAlpha (0.35f));
        g.fillEllipse (led.expanded (6.0f));
    }
    g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xffff8a8a) : juce::Colour (0xff5a1a1a), led.getX(), led.getY(),
                                             on ? juce::Colour (0xffd01010) : juce::Colour (0xff2a0808), led.getRight(), led.getBottom(), false));
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (led, 1.2f);
}

void PedalFace::resized()
{
    auto area = getLocalBounds().reduced (18, 0);
    area.removeFromTop (28);

    // Knobs first, then selectors, three per row (two rows at most).
    juce::Array<juce::Component*> list;
    for (auto* k : knobs) list.add (k);
    for (auto* s : selectors) list.add (s);

    const int cellW = 100, rowH = 118;
    const int perRow = list.size() == 4 ? 2 : 3;
    for (int i = 0; i < list.size(); i += perRow)
    {
        juce::Array<juce::Component*> row;
        for (int j = i; j < juce::jmin (list.size(), i + perRow); ++j)
            row.add (list[j]);
        layoutRow (row, area.removeFromTop (rowH), cellW);
    }

    const auto bottom = getLocalBounds().removeFromBottom (150);
    footswitch.setBounds (bottom.withSizeKeepingCentre (86, 86).translated (0, 18));
    ledArea = juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ (float) getWidth() * 0.5f, (float) bottom.getY() + 6.0f });

    // Centre the name plate in the space left between the controls and the LED.
    const int nameHeight = 72;
    const int top = area.getY() + 6, limit = bottom.getY() - 10;
    nameArea = { 16, top + juce::jmax (0, (limit - top - nameHeight) / 2), getWidth() - 32, nameHeight };
}

//==============================================================================
PedalEditor::PedalEditor (PedalProcessor& p)
    : AudioProcessorEditor (p),
      pedal (p),
      lookAndFeel (AmpTheme::pedal (juce::Colour (p.getEffectInfo().colour))),
      face (p.getEffectInfo(), p.getParameters(), {}, PedalProcessor::bypassId)
{
    setLookAndFeel (&lookAndFeel);
    content.addAndMakeVisible (face);
    content.setSize (baseWidth, baseHeight);
    face.setTopLeftPosition (24, 24);
    addAndMakeVisible (content);
    sendLookAndFeelChange();

    const double savedScale = (double) pedal.getParameters().state.getProperty (PedalProcessor::uiScaleProperty, 1.0);
    const double scale = juce::jlimit (0.6, 2.0, savedScale);
    setResizable (true, true);
    setResizeLimits ((int) (baseWidth * 0.6), (int) (baseHeight * 0.6), baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (juce::roundToInt (baseWidth * scale), juce::roundToInt (baseHeight * scale));
}

PedalEditor::~PedalEditor()
{
    setLookAndFeel (nullptr);
}

void PedalEditor::paint (juce::Graphics& g)
{
    AmpLookAndFeel::paintCovering (g, getLocalBounds(), lookAndFeel.getTheme());
}

void PedalEditor::resized()
{
    const float scale = (float) getWidth() / (float) baseWidth;
    content.setBounds (0, 0, baseWidth, baseHeight);
    content.setTransform (juce::AffineTransform::scale (scale));
    pedal.getParameters().state.setProperty (PedalProcessor::uiScaleProperty, scale, nullptr);
}

} // namespace oap::ui
