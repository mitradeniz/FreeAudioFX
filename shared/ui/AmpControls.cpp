#include "AmpControls.h"

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
}

//==============================================================================
AmpKnob::AmpKnob (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId, const juce::String& captionText)
    : caption (captionText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (220);
    slider.setVelocityModeParameters (0.25, 1, 0.0, true, juce::ModifierKeys::shiftModifier); // Shift = fine adjust
    slider.setPopupDisplayEnabled (false, false, nullptr);
    slider.setTitle (captionText);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterId, slider);

    if (auto* p = state.getParameter (parameterId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void AmpKnob::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    g.setFont (AmpLookAndFeel::font (12.0f, true).withExtraKerningFactor (0.08f));
    g.setColour (t.panelText);
    g.drawFittedText (caption.toUpperCase(), getLocalBounds().removeFromTop (18), juce::Justification::centred, 1, 0.75f);
}

void AmpKnob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (18);
    slider.setBounds (r);
}

//==============================================================================
AmpSwitch::AmpSwitch (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId, const juce::String& captionText)
    : caption (captionText)
{
    button.setTitle (captionText);
    button.setClickingTogglesState (true);
    button.onStateChange = [this] { repaint(); };
    addAndMakeVisible (button);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, parameterId, button);
}

void AmpSwitch::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    auto r = getLocalBounds();
    g.setFont (AmpLookAndFeel::font (12.0f, true).withExtraKerningFactor (0.08f));
    g.setColour (t.panelText);
    g.drawFittedText (caption.toUpperCase(), r.removeFromTop (18), juce::Justification::centred, 1, 0.75f);

    g.setFont (AmpLookAndFeel::font (12.5f));
    g.setColour (t.panelText.withAlpha (0.9f));
    g.drawText (button.getToggleState() ? "On" : "Off", r.removeFromBottom (18), juce::Justification::centred, false);
}

void AmpSwitch::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (18);
    r.removeFromBottom (18);
    button.setBounds (r.withSizeKeepingCentre (44, juce::jmin (r.getHeight(), 70)));
}

//==============================================================================
CabinetPanel::CabinetPanel (AmpProcessorBase& p) : processor (p)
{
    cabinetBox.addItemList (dsp::CabinetSection::getTypeNames(), 1);
    cabinetBox.setTitle ("Cabinet");
    addAndMakeVisible (cabinetBox);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.getParameters(), processor.getCabinetParameterId(), cabinetBox);

    loadButton.setTooltip ("Load a speaker cabinet impulse response (.wav / .aiff)");
    loadButton.onClick = [this] { chooseImpulseResponse(); };
    addAndMakeVisible (loadButton);

    clearButton.setTooltip ("Forget the loaded impulse response");
    clearButton.onClick = [this]
    {
        processor.clearImpulseResponse();
        lastError.clear();
        updateStatus();
    };
    addAndMakeVisible (clearButton);

    status.setFont (AmpLookAndFeel::font (13.0f));
    status.setJustificationType (juce::Justification::centredLeft);
    status.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (status);

    updateStatus();
    startTimerHz (5);
}

CabinetPanel::~CabinetPanel() { stopTimer(); }

void CabinetPanel::paint (juce::Graphics& g)
{
    const auto& t = themeOf (*this);
    auto r = getLocalBounds().toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

    g.setFont (AmpLookAndFeel::font (11.0f, true).withExtraKerningFactor (0.2f));
    g.setColour (t.headerText.withAlpha (0.6f));
    g.drawText ("CABINET", getLocalBounds().removeFromLeft (92).reduced (14, 0), juce::Justification::centredLeft, false);
}

void CabinetPanel::resized()
{
    auto r = getLocalBounds().reduced (10, 0);
    r.removeFromLeft (82);
    const int h = 30;
    cabinetBox.setBounds (r.removeFromLeft (170).withSizeKeepingCentre (170, h));
    r.removeFromLeft (12);
    loadButton.setBounds (r.removeFromLeft (100).withSizeKeepingCentre (100, h));
    r.removeFromLeft (6);
    clearButton.setBounds (r.removeFromLeft (64).withSizeKeepingCentre (64, h));
    r.removeFromLeft (14);
    status.setBounds (r);
}

void CabinetPanel::timerCallback() { updateStatus(); }

void CabinetPanel::updateStatus()
{
    const auto& t = themeOf (*this);
    const auto file = processor.getImpulseResponseFile();
    const bool loaded = processor.isImpulseResponseLoaded();
    const bool userSelected = cabinetBox.getSelectedId() == dsp::CabinetSection::UserIR + 1;

    juce::String text;
    juce::Colour colour = t.headerText.withAlpha (0.75f);

    if (lastError.isNotEmpty())
    {
        text = lastError;
        colour = juce::Colour (0xffff6b6b);
    }
    else if (loaded)
    {
        text = "IR: " + file.getFileNameWithoutExtension();
        if (! userSelected)
            text << "  (select User IR to hear it)";
        else if (processor.getActiveCabinet() != dsp::CabinetSection::UserIR)
            text << "  (loading...)";
    }
    else if (file != juce::File())
    {
        text = "IR file not found: " + file.getFileName();
        colour = juce::Colour (0xffffb347);
    }
    else if (userSelected)
    {
        text = "No IR loaded - using the 4x12 voicing";
        colour = juce::Colour (0xffffb347);
    }
    else
    {
        text = "Built-in cabinet voicing";
        colour = t.headerText.withAlpha (0.5f);
    }

    if (status.getText() != text)
        status.setText (text, juce::dontSendNotification);

    status.setColour (juce::Label::textColourId, colour);
    clearButton.setEnabled (loaded || file != juce::File());
}

void CabinetPanel::chooseImpulseResponse()
{
    auto start = processor.getImpulseResponseFile();
    if (! start.existsAsFile())
        start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

    chooser = std::make_unique<juce::FileChooser> ("Load a cabinet impulse response", start, "*.wav;*.aif;*.aiff;*.flac");

    juce::Component::SafePointer<CabinetPanel> safeThis (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safeThis] (const juce::FileChooser& fc)
                          {
                              if (safeThis == nullptr)
                                  return;

                              const auto file = fc.getResult();
                              if (file == juce::File())
                                  return;

                              safeThis->lastError = safeThis->processor.loadImpulseResponse (file);
                              safeThis->updateStatus();
                          });
}

//==============================================================================
AmpPanel::AmpPanel (AmpProcessorBase& p, const AmpTheme& t, Branding b, int width, int height)
    : theme (t), branding (std::move (b)), cabinetPanel (p)
{
    addAndMakeVisible (cabinetPanel);
    setSize (width, height);
}

void AmpPanel::addGroup (const juce::String& caption, std::vector<juce::Component*> controls)
{
    for (auto* c : controls)
        addAndMakeVisible (c);

    groups.push_back ({ caption, std::move (controls), {} });
    resized();
}

juce::Rectangle<int> AmpPanel::getFaceplateArea() const
{
    return { 18, 80, getWidth() - 36, getHeight() - 80 - 88 };
}

juce::Rectangle<int> AmpPanel::getCabinetStripArea() const
{
    return { 18, getHeight() - 70, getWidth() - 36, 52 };
}

void AmpPanel::paint (juce::Graphics& g)
{
    AmpLookAndFeel::paintCovering (g, getLocalBounds(), theme);

    // logo
    auto header = getLocalBounds().removeFromTop (80).reduced (30, 0);
    {
        const auto big = AmpLookAndFeel::font (34.0f, true).withExtraKerningFactor (0.08f);
        const float accentW = juce::GlyphArrangement::getStringWidth (big, branding.modelAccent);
        const float baseline = 46.0f;

        g.setFont (big);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawSingleLineText (branding.modelAccent, header.getX() + 2, (int) baseline + 2);
        g.setColour (theme.accent);
        g.drawSingleLineText (branding.modelAccent, header.getX(), (int) baseline);
        g.setColour (theme.headerText);
        g.drawSingleLineText (" " + branding.modelRest, header.getX() + (int) accentW, (int) baseline);

        g.setFont (AmpLookAndFeel::font (11.5f, true).withExtraKerningFactor (0.25f));
        g.setColour (theme.headerText.withAlpha (0.55f));
        g.drawSingleLineText (branding.tagline, header.getX() + 2, (int) baseline + 20);
    }

    // brand + pilot light
    {
        auto right = header.removeFromRight (260);
        const juce::Point<float> jewel ((float) right.getRight() - 14.0f, 40.0f);

        g.setColour (theme.accent.withAlpha (0.25f));
        g.fillEllipse (juce::Rectangle<float> (30.0f, 30.0f).withCentre (jewel));
        g.setGradientFill (juce::ColourGradient (theme.accent.brighter (0.8f), jewel.x - 3, jewel.y - 4,
                                                 theme.accent.darker (0.3f), jewel.x + 6, jewel.y + 7, true));
        g.fillEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre (jewel));
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (jewel), 2.0f);

        g.setFont (AmpLookAndFeel::font (11.5f, true).withExtraKerningFactor (0.25f));
        g.setColour (theme.headerText.withAlpha (0.65f));
        g.drawText ("OPEN AUDIO PLUGINS", right.withTrimmedRight (36), juce::Justification::centredRight, false);
    }

    const auto plate = getFaceplateArea().toFloat();
    AmpLookAndFeel::paintFaceplate (g, plate, theme);

    for (const auto& grp : groups)
    {
        AmpLookAndFeel::paintGroupCaption (g, grp.bounds.toFloat().withHeight (20.0f).translated (0.0f, -22.0f),
                                           grp.caption, theme);
    }

    // separators between groups
    g.setColour (theme.panelEdge.withAlpha (0.35f));
    for (size_t i = 1; i < groups.size(); ++i)
    {
        const float x = (float) (groups[i - 1].bounds.getRight() + groups[i].bounds.getX()) * 0.5f;
        g.drawLine (x, plate.getY() + 26.0f, x, plate.getBottom() - 26.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine (x + 1.0f, plate.getY() + 26.0f, x + 1.0f, plate.getBottom() - 26.0f, 1.0f);
        g.setColour (theme.panelEdge.withAlpha (0.35f));
    }
}

void AmpPanel::resized()
{
    cabinetPanel.setBounds (getCabinetStripArea());

    if (groups.empty())
        return;

    auto inner = getFaceplateArea().reduced (26, 0);
    inner.removeFromTop (40);   // group captions
    inner.removeFromBottom (18);

    size_t totalControls = 0;
    for (const auto& grp : groups)
        totalControls += grp.controls.size();

    const int gap = 26;
    const int columnWidth = (inner.getWidth() - gap * ((int) groups.size() - 1)) / (int) std::max<size_t> (1, totalControls);
    int x = inner.getX();

    for (auto& grp : groups)
    {
        const int w = columnWidth * (int) grp.controls.size();
        grp.bounds = { x, inner.getY(), w, inner.getHeight() };

        int cx = x;
        for (auto* c : grp.controls)
        {
            c->setBounds (juce::Rectangle<int> (cx, inner.getY(), columnWidth, inner.getHeight()).reduced (3, 0));
            cx += columnWidth;
        }

        x += w + gap;
    }
}

//==============================================================================
AmpEditor::AmpEditor (AmpProcessorBase& p, const AmpTheme& theme)
    : AudioProcessorEditor (p), lookAndFeel (theme), ampProcessor (p)
{
    setLookAndFeel (&lookAndFeel);
}

AmpEditor::~AmpEditor()
{
    panel.reset();
    setLookAndFeel (nullptr);
}

void AmpEditor::setPanel (std::unique_ptr<AmpPanel> newPanel)
{
    panel = std::move (newPanel);
    baseWidth = panel->getWidth();
    baseHeight = panel->getHeight();
    addAndMakeVisible (*panel);

    // The panel's controls were created before they had a parent, so their
    // sub-components (e.g. slider text boxes) must be rebuilt with our look.
    sendLookAndFeelChange();

    const double savedScale = (double) ampProcessor.getParameters().state.getProperty (AmpProcessorBase::uiScaleProperty, 1.0);
    const double scale = juce::jlimit (0.6, 2.0, savedScale);

    setResizable (true, true);
    setResizeLimits ((int) (baseWidth * 0.6), (int) (baseHeight * 0.6), baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);

    setSize (juce::roundToInt (baseWidth * scale), juce::roundToInt (baseHeight * scale));
}

void AmpEditor::paint (juce::Graphics& g)
{
    g.fillAll (lookAndFeel.getTheme().background);
}

void AmpEditor::resized()
{
    if (panel == nullptr)
        return;

    const float scale = (float) getWidth() / (float) baseWidth;
    panel->setBounds (0, 0, baseWidth, baseHeight);
    panel->setTransform (juce::AffineTransform::scale (scale));
    ampProcessor.getParameters().state.setProperty (AmpProcessorBase::uiScaleProperty, scale, nullptr);
}

} // namespace oap::ui
