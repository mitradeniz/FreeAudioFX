#include "PedalboardEditor.h"

namespace oap::ui
{

namespace
{
    AmpTheme boardTheme()
    {
        return AmpTheme::custom (juce::Colour (0xff141518), juce::Colour (0xff2b2c31), juce::Colour (0xff1d1e21),
                                 juce::Colour (0xffffb000), false);
    }

    const AmpTheme& themeOf (const juce::Component& c)
    {
        if (auto* lnf = dynamic_cast<AmpLookAndFeel*> (&c.getLookAndFeel()))
            return lnf->getTheme();
        static const AmpTheme fallback = boardTheme();
        return fallback;
    }
}

//==============================================================================
PedalSlot::MiniKnob::MiniKnob (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 70, 15);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (200);
    slider.setVelocityModeParameters (0.25, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterId, slider);
}

void PedalSlot::MiniKnob::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    g.setFont (AmpLookAndFeel::font (10.5f, true));
    g.setColour (t.panelText);
    g.drawFittedText (caption.toUpperCase(), getLocalBounds().removeFromTop (14), juce::Justification::centred, 1, 0.7f);
}

void PedalSlot::MiniKnob::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (14));
}

//==============================================================================
PedalSlot::PedalSlot (PedalboardProcessor& p, int slotIndex) : board (p), index (slotIndex)
{
    picker.setTitle ("Slot " + juce::String (index + 1) + " pedal");
    picker.addItem ("Empty", 1);
    bool advancedHeading = false;
    picker.addSectionHeading ("Everyday pedals");
    const auto& all = fx::allEffects();
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (all[i]->advanced && ! advancedHeading)
        {
            picker.addSectionHeading ("Advanced series");
            advancedHeading = true;
        }
        picker.addItem (all[i]->name, (int) i + 2);
    }
    picker.onChange = [this] { effectChosen(); };
    addAndMakeVisible (picker);

    moveLeft.setTooltip ("Move this pedal one place earlier in the chain");
    moveRight.setTooltip ("Move this pedal one place later in the chain");
    moveLeft.onClick = [this] { board.swapSlots (index, index - 1); };
    moveRight.onClick = [this] { board.swapSlots (index, index + 1); };
    moveLeft.setEnabled (index > 0);
    moveRight.setEnabled (index < PedalboardProcessor::numSlots - 1);
    addAndMakeVisible (moveLeft);
    addAndMakeVisible (moveRight);

    for (int k = 0; k < PedalboardProcessor::knobsPerSlot; ++k)
        addChildComponent (knobs.add (new MiniKnob (board.getParameters(), PedalboardProcessor::knobId (index, k))));

    // The board stores "on"; the footswitch's toggle state means "bypassed".
    footswitch.setClickingTogglesState (true);
    footswitch.onClick = [this]
    {
        if (auto* on = board.getParameters().getParameter (PedalboardProcessor::onId (index)))
            on->setValueNotifyingHost (footswitch.getToggleState() ? 0.0f : 1.0f);
    };
    // The LED is drawn by the slot, not by the footswitch, so it must be
    // repainted whenever the footswitch changes state.
    footswitch.onStateChange = [this] { repaint (ledArea.toNearestInt().expanded (8)); };
    addAndMakeVisible (footswitch);

    refresh();
}

PedalSlot::~PedalSlot()
{
    setLookAndFeel (nullptr);
}

void PedalSlot::effectChosen()
{
    if (! initialised)
        return;
    const int effectIndex = picker.getSelectedId() - 2; // -1 = empty
    board.setSlotEffect (index, effectIndex);
    refresh();
}

void PedalSlot::refresh()
{
    const auto* effect = board.getSlotEffect (index);
    const bool on = board.getParameters().getRawParameterValue (PedalboardProcessor::onId (index))->load() > 0.5f;
    if (footswitch.getToggleState() == on)
    {
        footswitch.setToggleState (! on, juce::dontSendNotification);
        repaint (ledArea.toNearestInt().expanded (8));
    }

    if (effect == shownEffect && initialised)
        return;

    shownEffect = effect;
    const auto& all = fx::allEffects();
    int id = 1;
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i] == effect)
            id = (int) i + 2;
    picker.setSelectedId (id, juce::dontSendNotification);

    auto newLook = std::make_unique<AmpLookAndFeel> (effect != nullptr ? AmpTheme::pedal (juce::Colour (effect->colour))
                                                                       : boardTheme());
    setLookAndFeel (newLook.get());
    lookAndFeel = std::move (newLook);

    for (int k = 0; k < knobs.size(); ++k)
    {
        auto* knob = knobs[k];
        const bool used = effect != nullptr && (size_t) k < effect->params.size();
        knob->setVisible (used);
        if (used)
        {
            const auto& spec = effect->params[(size_t) k];
            knob->caption = spec.name;
            knob->slider.setDoubleClickReturnValue (true, spec.toNormalised (spec.def));
            knob->slider.updateText();
            knob->repaint();
        }
    }

    footswitch.setVisible (effect != nullptr);
    initialised = true;
    resized();
    repaint();
}

void PedalSlot::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto body = getLocalBounds().toFloat().reduced (3.0f);

    if (shownEffect == nullptr)
    {
        juce::Path outline;
        outline.addRoundedRectangle (body, 12.0f);
        juce::Path dashed;
        const float dashes[] = { 6.0f, 5.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, outline, dashes, 2);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillPath (dashed);
        g.setFont (AmpLookAndFeel::font (13.0f, true));
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawFittedText ("EMPTY SLOT\nchoose a pedal above", body.toNearestInt().withTrimmedTop (120).removeFromTop (60),
                          juce::Justification::centred, 2);
        return;
    }

    juce::Path shape;
    shape.addRoundedRectangle (body, 12.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 14, { 0, 6 }).drawForPath (g, shape);
    g.setGradientFill (juce::ColourGradient (t.panelTop, body.getX(), body.getY(), t.panelBottom, body.getRight(), body.getBottom(), false));
    g.fillPath (shape);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), body.getX(), body.getY(),
                                             juce::Colours::transparentWhite, body.getX(), body.getY() + 60.0f, false));
    g.fillPath (shape);
    g.setColour (t.panelEdge.withAlpha (0.8f));
    g.strokePath (shape, juce::PathStrokeType (1.2f));

    // slot number
    g.setFont (AmpLookAndFeel::font (12.0f, true));
    g.setColour (t.panelTextDim);
    g.drawText (juce::String (index + 1), juce::Rectangle<int> (0, 8, getWidth(), 18), juce::Justification::centred, false);

    // LED
    const bool on = ! footswitch.getToggleState();
    if (on)
    {
        g.setColour (juce::Colour (0xffff2a2a).withAlpha (0.35f));
        g.fillEllipse (ledArea.expanded (5.0f));
    }
    g.setColour (on ? juce::Colour (0xffff3b3b) : juce::Colour (0xff4a1414));
    g.fillEllipse (ledArea);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (ledArea, 1.0f);
}

void PedalSlot::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    auto top = r.removeFromTop (22);
    moveLeft.setBounds (top.removeFromLeft (26));
    moveRight.setBounds (top.removeFromRight (26));
    r.removeFromTop (6);
    picker.setBounds (r.removeFromTop (28));
    r.removeFromTop (8);

    const int cellW = r.getWidth() / 2, cellH = 84;
    for (int k = 0; k < knobs.size(); ++k)
        knobs[k]->setBounds (r.getX() + (k % 2) * cellW, r.getY() + (k / 2) * cellH, cellW, cellH);

    auto bottom = getLocalBounds().removeFromBottom (110);
    footswitch.setBounds (bottom.withSizeKeepingCentre (64, 64).translated (0, 14));
    ledArea = juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ (float) getWidth() * 0.5f, (float) bottom.getY() + 4.0f });
}

//==============================================================================
void PedalboardEditor::Content::paint (juce::Graphics& g)
{
    AmpLookAndFeel::paintCovering (g, getLocalBounds(), theme);

    const float baseline = 52.0f;
    const auto big = AmpLookAndFeel::font (34.0f, true).withExtraKerningFactor (0.08f);
    g.setFont (big);
    g.setColour (theme.accent);
    g.drawSingleLineText ("PEDAL", 30, (int) baseline);
    g.setColour (theme.headerText);
    g.drawSingleLineText ("BOARD", 30 + (int) juce::GlyphArrangement::getStringWidth (big, "PEDAL "), (int) baseline);

    g.setFont (AmpLookAndFeel::font (11.5f, true).withExtraKerningFactor (0.25f));
    g.setColour (theme.headerText.withAlpha (0.55f));
    g.drawSingleLineText ("8 SLOTS  /  20 PEDALS  /  SIGNAL FLOWS LEFT TO RIGHT", 32, (int) baseline + 22);

    g.setFont (AmpLookAndFeel::font (12.0f));
    g.setColour (theme.headerText.withAlpha (0.45f));
    g.drawText ("Pick a pedal from each slot's menu. Use < and > to change the order. Double-click a knob to reset it.",
                juce::Rectangle<int> (0, getHeight() - 30, getWidth(), 20), juce::Justification::centred, false);
}

PedalboardEditor::PedalboardEditor (PedalboardProcessor& p)
    : AudioProcessorEditor (p),
      board (p),
      lookAndFeel (boardTheme()),
      inputKnob (p.getParameters(), "input", "Input"),
      outputKnob (p.getParameters(), "output", "Output")
{
    setLookAndFeel (&lookAndFeel);
    content.theme = boardTheme();
    content.setSize (baseWidth, baseHeight);
    addAndMakeVisible (content);

    for (int s = 0; s < PedalboardProcessor::numSlots; ++s)
        content.addAndMakeVisible (slots.add (new PedalSlot (board, s)));

    content.addAndMakeVisible (inputKnob);
    content.addAndMakeVisible (outputKnob);

    const int slotW = 154, gap = 5, top = 104;
    const int total = PedalboardProcessor::numSlots * slotW + (PedalboardProcessor::numSlots - 1) * gap;
    int x = (baseWidth - total) / 2;
    for (auto* slot : slots)
    {
        slot->setBounds (x, top, slotW, 470);
        x += slotW + gap;
    }
    inputKnob.setBounds (baseWidth - 210, 6, 90, 96);
    outputKnob.setBounds (baseWidth - 112, 6, 90, 96);

    sendLookAndFeelChange();
    for (auto* slot : slots)
        slot->refresh();

    const double savedScale = (double) board.getParameters().state.getProperty (PedalboardProcessor::uiScaleProperty, 1.0);
    const double scale = juce::jlimit (0.5, 2.0, savedScale);
    setResizable (true, true);
    setResizeLimits ((int) (baseWidth * 0.5), (int) (baseHeight * 0.5), baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (juce::roundToInt (baseWidth * scale), juce::roundToInt (baseHeight * scale));

    startTimerHz (10);
}

PedalboardEditor::~PedalboardEditor()
{
    stopTimer();
    slots.clear();
    setLookAndFeel (nullptr);
}

void PedalboardEditor::timerCallback()
{
    for (auto* slot : slots)
        slot->refresh();
}

void PedalboardEditor::paint (juce::Graphics& g)
{
    g.fillAll (content.theme.background);
}

void PedalboardEditor::resized()
{
    const float scale = (float) getWidth() / (float) baseWidth;
    content.setBounds (0, 0, baseWidth, baseHeight);
    content.setTransform (juce::AffineTransform::scale (scale));
    board.getParameters().state.setProperty (PedalboardProcessor::uiScaleProperty, scale, nullptr);
}

} // namespace oap::ui
