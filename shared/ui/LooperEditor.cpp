#include "LooperEditor.h"

#include <cmath>

namespace oap::ui
{

namespace
{
    using juce::Colour;
    using looper::State;

    const Colour recordColour  { 0xffff4f52 };
    const Colour playColour    { 0xff3ddc84 };
    const Colour overdubColour { 0xffffb224 };
    const Colour idleColour    { 0xff8c95a1 };
    const Colour screenColour  { 0xff090a0d };

    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    // Design-size layout (the whole editor is scaled with the window).
    const juce::Rectangle<int> leftPanelArea { 20, 70, 284, 670 };
    const juce::Rectangle<int> displayArea   { 316, 70, 704, 230 };
    const juce::Rectangle<int> controlsArea  { 316, 312, 704, 428 };

    AmpTheme looperTheme()
    {
        return AmpTheme::custom (Colour (0xff111318), Colour (0xff272b33), Colour (0xff1a1d23), Colour (0xff4db8ff), false);
    }

    juce::String utf8 (const char* text) { return juce::String (juce::CharPointer_UTF8 (text)); }

    Colour stateColour (const looper::Status& s)
    {
        switch (s.state)
        {
            case State::Armed:
            case State::Recording:    return recordColour;
            case State::Playing:      return playColour;
            case State::Overdubbing:  return overdubColour;
            case State::Stopped:      return idleColour;
            case State::Empty:
            default:                  return idleColour.withMultipliedBrightness (0.7f);
        }
    }

    juce::String stateName (const looper::Status& s)
    {
        if (s.autoRunning && s.state == State::Armed)
            return "COUNT-IN";
        if (s.autoLayer > 0 && s.state == State::Overdubbing)
            return "AUTO LAYER " + juce::String (s.autoLayer + 1) + " OF " + juce::String (s.autoLayers + 1);

        switch (s.state)
        {
            case State::Armed:        return "ARMED";
            case State::Recording:    return "RECORDING";
            case State::Playing:      return s.stopping ? "STOPPING" : "PLAYING";
            case State::Overdubbing:  return "OVERDUBBING";
            case State::Stopped:      return "STOPPED";
            case State::Empty:
            default:                  return "EMPTY";
        }
    }

    /** Minutes, seconds and tenths, rounded down so the clock never runs ahead. */
    juce::String timeText (double seconds)
    {
        const auto tenths = (juce::int64) std::floor (juce::jmax (0.0, seconds) * 10.0 + 1.0e-6);
        return juce::String (tenths / 600) + ":" + juce::String ((tenths / 10) % 60).paddedLeft ('0', 2)
             + "." + juce::String (tenths % 10);
    }

    /** "4 bars" or "2 beats" for a length given in beats - or nothing, when it isn't a whole number of either. */
    juce::String musicalLength (double beats, double beatsPerBar)
    {
        const auto plural = [] (double n, const char* unit)
        {
            return juce::String (juce::roundToInt (n)) + " " + unit + (juce::roundToInt (n) == 1 ? "" : "s");
        };

        const double bars = beats / juce::jmax (1.0, beatsPerBar);
        if (bars >= 0.99 && std::abs (bars - std::round (bars)) < 0.02)
            return plural (bars, "bar");
        if (beats >= 0.99 && std::abs (beats - std::round (beats)) < 0.02)
            return plural (beats, "beat");
        return {};
    }

    /** One thin bar per three pixels, mirrored around the centre line. */
    void drawBars (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<float>& values,
                   float widthFraction, Colour colour, bool skipSilence)
    {
        const int count = (int) values.size();
        if (count == 0 || widthFraction <= 0.0f)
            return;

        constexpr float step = 3.0f;
        const int columns = juce::jmax (1, (int) (area.getWidth() * juce::jmin (1.0f, widthFraction) / step));
        const float centre = area.getCentreY(), half = area.getHeight() * 0.5f;

        juce::RectangleList<float> bars;
        for (int column = 0; column < columns; ++column)
        {
            const int first = (int) ((juce::int64) column * count / columns);
            const int last = juce::jmax (first + 1, (int) ((juce::int64) (column + 1) * count / columns));

            float peak = 0.0f;
            for (int i = first; i < last && i < count; ++i)
                peak = juce::jmax (peak, values[(size_t) i]);

            if (skipSilence && peak < 0.002f)
                continue;

            // square root: quiet playing stays visible
            const float h = juce::jmax (0.75f, half * std::sqrt (juce::jlimit (0.0f, 1.0f, peak)));
            bars.addWithoutMerging ({ area.getX() + (float) column * step, centre - h, step - 1.0f, 2.0f * h });
        }

        g.setColour (colour);
        g.fillRectList (bars);
    }

    const juce::String& audioWildcard()
    {
        static const juce::String wildcard = []
        {
            juce::AudioFormatManager formats;
            formats.registerBasicFormats();
            return formats.getWildcardForAllFormats();
        }();
        return wildcard;
    }

    constexpr const char* lengthNames[] = { "", "1 Beat", "2 Beats", "1 Bar", "2 Bars", "4 Bars", "8 Bars", "16 Bars" };

    /** The beat a musician counts in this time signature, in quarter notes: a dotted quarter in
        6/8, 9/8 and 12/8, an eighth in other x/8 bars, otherwise the note value of the denominator. */
    double countedBeat (int numerator, int denominator)
    {
        if (denominator <= 0)
            return 1.0;
        const double unit = 4.0 / denominator;
        return (denominator >= 8 && numerator > 3 && numerator % 3 == 0) ? 3.0 * unit : unit;
    }

    /** A captioned group of controls in a row of settings: each control with its width. */
    struct RowGroup
    {
        const char* caption;
        std::vector<std::pair<juce::Component*, int>> controls;
    };
}

//==============================================================================
/** Everything that doesn't move, drawn once into a cached image. */
class LooperEditor::Backdrop : public juce::Component
{
public:
    explicit Backdrop (LooperEditor& e) : owner (e)
    {
        setInterceptsMouseClicks (false, false);
        setBufferedToImage (true);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = owner.lookAndFeel.getTheme();
        const auto bounds = getLocalBounds().toFloat();

        g.setGradientFill (juce::ColourGradient (t.background.brighter (0.08f), 0.0f, 0.0f,
                                                 t.background.darker (0.45f), 0.0f, bounds.getBottom(), false));
        g.fillAll();

        // logo
        const auto big = AmpLookAndFeel::font (33.0f, true).withExtraKerningFactor (0.1f);
        g.setFont (big);
        g.setColour (t.accent);
        g.drawSingleLineText ("LOOP", 26, 41);
        g.setColour (t.headerText);
        g.drawSingleLineText ("ER", 26 + (int) juce::GlyphArrangement::getStringWidth (big, "LOOP"), 41);

        g.setFont (AmpLookAndFeel::font (11.0f, true).withExtraKerningFactor (0.24f));
        g.setColour (t.headerText.withAlpha (0.5f));
        g.drawSingleLineText ("RECORD  /  OVERDUB  /  UNDO  /  IN TIME WITH YOUR PROJECT", 28, 58);

        for (const auto& area : { leftPanelArea, controlsArea })
            paintPanel (g, area.toFloat(), t);

        for (const auto& group : owner.groups)
            AmpLookAndFeel::paintGroupCaption (g, group.bounds.toFloat(), group.caption, t);
    }

private:
    static void paintPanel (juce::Graphics& g, juce::Rectangle<float> area, const AmpTheme& t)
    {
        juce::Path outline;
        outline.addRoundedRectangle (area, 10.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 14, { 0, 5 }).drawForPath (g, outline);

        g.setGradientFill (juce::ColourGradient (t.panelTop, area.getX(), area.getY(),
                                                 t.panelBottom, area.getX(), area.getBottom(), false));
        g.fillPath (outline);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawLine (area.getX() + 10.0f, area.getY() + 1.0f, area.getRight() - 10.0f, area.getY() + 1.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.strokePath (outline, juce::PathStrokeType (1.2f));
    }

    LooperEditor& owner;
};

//==============================================================================
/** The big round switch. Works like a looper's footswitch: click, double-click
    and hold. The ring shows where the loop is; the middle shows what a click
    does next. */
class LooperEditor::MainButton : public juce::Component
{
public:
    explicit MainButton (LooperEditor& e) : owner (e)
    {
        setTitle ("Loop switch");
        setRepaintsOnMouseActivity (true);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    bool hitTest (int x, int y) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        return c.getDistanceFrom ({ (float) x, (float) y }) <= ringRadius() + 6.0f;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || down)
            return;

        const auto state = owner.view.status.state;
        holdMatters = state == State::Playing || state == State::Overdubbing || state == State::Stopped;
        down = true;
        downAt = juce::Time::getMillisecondCounter();
        owner.looper.sendSwitch (LooperProcessor::Switch::mainDown);
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! down)
            return;

        down = false;
        owner.looper.sendSwitch (LooperProcessor::Switch::mainUp);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& v = owner.view;
        const auto& s = v.status;
        const auto& t = owner.lookAndFeel.getTheme();
        const auto c = getLocalBounds().toFloat().getCentre();
        const float ring = ringRadius(), ringWidth = 9.0f;
        const float disc = ring - 17.0f - (down ? 2.0f : 0.0f);
        const auto colour = stateColour (s);
        const auto now = juce::Time::getMillisecondCounter();
        const float blink = 0.5f + 0.5f * std::sin ((float) (now % 100000u) * 0.0125f);

        // ring: the loop as a circle
        juce::Path track;
        track.addCentredArc (c.x, c.y, ring, ring, 0.0f, 0.0f, twoPi, true);
        g.setColour (Colour (0xff0b0c0f));
        g.strokePath (track, juce::PathStrokeType (ringWidth + 5.0f));
        g.setColour (Colour (0xff2c3038));
        g.strokePath (track, juce::PathStrokeType (ringWidth));

        float from = 0.0f, to = 0.0f, alpha = 1.0f;

        switch (s.state)
        {
            case State::Recording:
                if (s.targetSeconds > 0.0)
                {
                    to = twoPi * (float) juce::jlimit (0.0, 1.0, s.lengthSeconds / s.targetSeconds);
                }
                else
                {
                    from = std::fmod ((float) (now % 100000u) * 0.004f, twoPi); // free length: goes round and round
                    to = from + 1.7f;
                }
                break;

            case State::Armed:
                if (s.countInLength > 0.0)
                {
                    to = twoPi * (float) juce::jlimit (0.0, 1.0, 1.0 - s.countIn / s.countInLength); // the count-in fills the ring
                }
                else
                {
                    to = twoPi;
                    alpha = 0.2f + 0.8f * blink;
                }
                break;

            case State::Playing:
            case State::Overdubbing:
                if (s.lengthSeconds > 0.0)
                    to = twoPi * (float) juce::jlimit (0.0, 1.0, s.positionSeconds / s.lengthSeconds);
                if (s.stopping)
                    alpha = 0.35f + 0.65f * blink;
                break;

            case State::Stopped:
                if (s.scheduled)
                {
                    to = twoPi;
                    alpha = 0.2f + 0.8f * blink;
                }
                break;

            case State::Empty:
            default:
                break;
        }

        if (to > from)
        {
            juce::Path arc;
            arc.addCentredArc (c.x, c.y, ring, ring, 0.0f, from, to, true);
            g.setColour (colour.withAlpha (0.22f * alpha));
            g.strokePath (arc, juce::PathStrokeType (ringWidth + 9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (colour.withAlpha (alpha));
            g.strokePath (arc, juce::PathStrokeType (ringWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // the switch itself
        const juce::Rectangle<float> discBounds (c.x - disc, c.y - disc, disc * 2.0f, disc * 2.0f);
        juce::Path body;
        body.addEllipse (discBounds);
        juce::DropShadow (juce::Colours::black.withAlpha (0.65f), down ? 8 : 16, { 0, down ? 2 : 6 }).drawForPath (g, body);
        g.setGradientFill (juce::ColourGradient (Colour (0xff3b404a), c.x, c.y - disc, Colour (0xff1a1d22), c.x, c.y + disc, false));
        g.fillPath (body);

        const bool lit = s.state == State::Recording || s.state == State::Overdubbing || s.state == State::Playing;
        if (lit)
        {
            g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.30f), c.x, c.y,
                                                     colour.withAlpha (0.0f), c.x, c.y + disc, true));
            g.fillPath (body);
        }

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), c.x, c.y - disc,
                                                 juce::Colours::transparentWhite, c.x, c.y - disc * 0.2f, false));
        g.fillEllipse (discBounds.reduced (disc * 0.12f, 0.0f).withHeight (disc * 0.9f));
        g.setColour (isMouseOver() ? t.accent.withAlpha (0.9f) : juce::Colours::black.withAlpha (0.8f));
        g.drawEllipse (discBounds, isMouseOver() ? 1.8f : 1.2f);

        // holding: fills up until the hold takes effect
        if (down && holdMatters)
        {
            const float held = juce::jlimit (0.0f, 1.0f, (float) (now - downAt) / 800.0f);
            juce::Path hold;
            hold.addCentredArc (c.x, c.y, disc - 7.0f, disc - 7.0f, 0.0f, 0.0f, twoPi * held, true);
            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.strokePath (hold, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // what a click does now
        enum class Action { record, overdub, play, cancel };
        auto action = Action::record;

        switch (s.state)
        {
            case State::Armed:        action = Action::cancel; break;
            case State::Recording:    action = v.overdubAfterRecord ? Action::overdub : Action::play; break;
            case State::Playing:      action = s.stopping ? Action::play : Action::overdub; break;
            case State::Overdubbing:  action = Action::play; break;
            case State::Stopped:      action = s.scheduled ? Action::cancel : Action::play; break;
            case State::Empty:
            default:                  break;
        }

        const juce::Point<float> icon (c.x, c.y - 15.0f);
        const float size = 17.0f;
        juce::String caption;

        const int count = s.state == State::Armed && s.countIn > 0.0
                              ? (int) std::ceil (s.countIn / countedBeat (s.numerator, s.denominator) - 1.0e-6) : 0;

        if (action == Action::record)
        {
            caption = v.autoLoop > 0 ? "AUTO LOOP" : "RECORD";
            g.setColour (recordColour);
            g.fillEllipse (icon.x - size, icon.y - size, size * 2.0f, size * 2.0f);
        }
        else if (count > 0)
        {
            // counting in: the number of beats to go
            caption = "CANCEL";
            g.setColour (recordColour);
            g.setFont (AmpLookAndFeel::font (46.0f, true));
            g.drawText (juce::String (count), juce::Rectangle<float> (80.0f, 50.0f).withCentre (icon.translated (0.0f, 2.0f)),
                        juce::Justification::centred, false);
        }
        else if (action == Action::overdub)
        {
            caption = "OVERDUB";
            g.setColour (overdubColour);
            g.fillEllipse (icon.x - size, icon.y - size, size * 2.0f, size * 2.0f);
            g.setColour (Colour (0xff1a1d22));
            g.fillRect (juce::Rectangle<float> (18.0f, 4.0f).withCentre (icon));
            g.fillRect (juce::Rectangle<float> (4.0f, 18.0f).withCentre (icon));
        }
        else if (action == Action::play)
        {
            caption = "PLAY";
            juce::Path triangle;
            triangle.addTriangle (icon.x - size * 0.75f, icon.y - size, icon.x - size * 0.75f, icon.y + size, icon.x + size, icon.y);
            g.setColour (playColour);
            g.fillPath (triangle);
        }
        else
        {
            caption = "CANCEL";
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.drawLine (icon.x - 13.0f, icon.y - 13.0f, icon.x + 13.0f, icon.y + 13.0f, 4.5f);
            g.drawLine (icon.x - 13.0f, icon.y + 13.0f, icon.x + 13.0f, icon.y - 13.0f, 4.5f);
        }

        g.setFont (AmpLookAndFeel::font (15.0f, true).withExtraKerningFactor (0.16f));
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.drawText (caption, juce::Rectangle<float> (disc * 2.0f, 22.0f).withCentre ({ c.x, c.y + 32.0f }),
                    juce::Justification::centred, false);
    }

private:
    float ringRadius() const { return (float) juce::jmin (getWidth(), getHeight()) * 0.5f - 9.0f; }

    LooperEditor& owner;
    bool down = false, holdMatters = false;
    juce::uint32 downAt = 0;
};

//==============================================================================
/** The screen: what the looper is doing, the loop's waveform with the play
    position, time, layers and tempo. */
class LooperEditor::Display : public juce::Component
{
public:
    explicit Display (LooperEditor& e) : owner (e) { setInterceptsMouseClicks (false, false); }

    void paint (juce::Graphics& g) override
    {
        const auto& v = owner.view;
        const auto& s = v.status;
        const auto& t = owner.lookAndFeel.getTheme();
        const auto bounds = getLocalBounds().toFloat();
        const auto colour = stateColour (s);
        const auto now = juce::Time::getMillisecondCounter();
        const float blink = 0.5f + 0.5f * std::sin ((float) (now % 100000u) * 0.0125f);

        g.setColour (screenColour);
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.04f), 0.0f, 0.0f,
                                                 juce::Colours::transparentWhite, 0.0f, 70.0f, false));
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (Colour (0xff2c3139));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);

        auto inner = bounds.reduced (16.0f, 12.0f);
        auto top = inner.removeFromTop (30.0f);
        auto bottom = inner.removeFromBottom (24.0f);
        const auto wave = inner.reduced (0.0f, 9.0f);

        //==============================================================================
        // top line: state and time
        {
            const auto name = stateName (s);
            const auto f = AmpLookAndFeel::font (13.0f, true).withExtraKerningFactor (0.16f);
            const float width = juce::GlyphArrangement::getStringWidth (f, name) + 40.0f;
            const auto pill = top.removeFromLeft (width).withSizeKeepingCentre (width, 24.0f);

            g.setColour (colour.withAlpha (0.16f));
            g.fillRoundedRectangle (pill, 12.0f);
            g.setColour (colour.withAlpha (0.6f));
            g.drawRoundedRectangle (pill.reduced (0.5f), 12.0f, 1.0f);

            const bool flashing = s.state == State::Armed || s.state == State::Recording || s.stopping;
            g.setColour (colour.withAlpha (flashing ? 0.35f + 0.65f * blink : 1.0f));
            g.fillEllipse (pill.getX() + 10.0f, pill.getCentreY() - 4.0f, 8.0f, 8.0f);
            g.setColour (colour.brighter (0.5f));
            g.setFont (f);
            g.drawText (name, pill.withTrimmedLeft (24.0f).withTrimmedRight (8.0f), juce::Justification::centred, false);

            juce::String time;
            if (s.state == State::Recording)
                time = timeText (s.lengthSeconds) + (s.targetSeconds > 0.0 ? "  /  " + timeText (s.targetSeconds) : juce::String());
            else if (s.state == State::Playing || s.state == State::Overdubbing || s.state == State::Stopped)
                time = timeText (s.positionSeconds) + "  /  " + timeText (s.lengthSeconds);

            g.setFont (AmpLookAndFeel::font (21.0f, true));
            g.setColour (t.panelText);
            g.drawText (time, top, juce::Justification::centredRight, false);
        }

        //==============================================================================
        // the loop
        const bool hasLoop = s.state == State::Playing || s.state == State::Overdubbing || s.state == State::Stopped;
        if (hasLoop || s.state == State::Recording)
        {
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.fillRect (wave.getX(), wave.getCentreY() - 0.5f, wave.getWidth(), 1.0f);
        }

        const double gridSeconds = s.state == State::Recording ? s.targetSeconds : (hasLoop ? s.lengthSeconds : 0.0);

        if (gridSeconds > 0.0 && s.loopBpm > 0.0)
        {
            // beat and bar lines
            const double beats = gridSeconds * s.loopBpm / 60.0;
            const double bars = beats / juce::jmax (1.0, s.beatsPerBar);

            // (a faint line behind the waveform, and a tick below it that a dense waveform can't hide)
            const auto lines = [&g, wave] (double count, float alpha, float tick)
            {
                if (count > 64.0)
                    return;

                for (int i = 1; (double) i < count - 0.01; ++i)
                {
                    const float x = wave.getX() + wave.getWidth() * (float) ((double) i / count) - 0.5f;
                    g.setColour (juce::Colours::white.withAlpha (alpha));
                    g.fillRect (x, wave.getY(), 1.0f, wave.getHeight());
                    g.setColour (juce::Colours::white.withAlpha (juce::jmin (1.0f, alpha * 3.5f)));
                    g.fillRect (x, wave.getBottom() + 2.0f, 1.0f, tick);
                }
            };

            lines (beats, 0.06f, 3.0f);
            lines (bars, 0.16f, 6.0f);
        }

        if (s.state == State::Armed && s.autoRunning)
        {
            // count-in: the beats to go, big
            const double beat = countedBeat (s.numerator, s.denominator);
            const int count = (int) std::ceil (s.countIn / beat - 1.0e-6);
            const float within = (float) (count * beat - s.countIn) / (float) beat; // 0 at each counted beat
            auto area = wave;

            g.setFont (AmpLookAndFeel::font (84.0f, true));
            g.setColour (recordColour.withAlpha (count > 0 ? 1.0f - 0.55f * within : 0.4f));
            g.drawText (count > 0 ? juce::String (count) : juce::String ("..."), area.removeFromTop (area.getHeight() * 0.72f),
                        juce::Justification::centred, false);

            g.setFont (AmpLookAndFeel::font (14.0f));
            g.setColour (t.panelTextDim);
            g.drawText ("Recording " + v.autoLength.toLowerCase() + " starts after the count", area, juce::Justification::centredTop, false);
        }
        else if (s.state == State::Empty || s.state == State::Armed)
        {
            juce::String text = "Click the big button and play  -  or drop an audio file here";
            if (s.state == State::Empty && v.autoLoop > 0)
                text = "Auto loop: click once, play after the count-in  -  " + v.autoLength.toLowerCase()
                     + (v.autoLoop > 1 ? ", " + juce::String (v.autoLoop) + " layers" : juce::String());
            else if (s.state == State::Armed)
                text = v.autoStart ? "Waiting for your first note..."
                                   : juce::String ("Recording starts on the next ") + (v.quantize == 1 ? "beat" : "bar") + "...";

            g.setFont (AmpLookAndFeel::font (15.0f));
            g.setColour (s.state == State::Armed ? recordColour.withAlpha (0.55f + 0.45f * blink) : t.panelTextDim);
            g.drawText (text, wave, juce::Justification::centred, false);
        }
        else if (s.state == State::Recording)
        {
            const float fraction = s.targetSeconds > 0.0 ? (float) juce::jlimit (0.0, 1.0, s.lengthSeconds / s.targetSeconds) : 1.0f;
            drawBars (g, wave, owner.live, fraction, recordColour.withAlpha (0.9f), false);

            if (s.targetSeconds > 0.0)
            {
                g.setColour (juce::Colours::white.withAlpha (0.9f));
                g.fillRect (wave.getX() + wave.getWidth() * fraction - 1.0f, wave.getY(), 2.0f, wave.getHeight());
            }
        }
        else
        {
            // (the picture arrives a moment after the loop has changed)
            const bool current = ! owner.peaks.empty() && std::abs (owner.peaksLength - s.lengthSeconds) < 0.002;
            // The loop in green; what an overdub is adding right now on top of it.
            if (current)
                drawBars (g, wave, owner.peaks, 1.0f,
                          s.state == State::Stopped ? idleColour.withAlpha (0.55f)
                                                    : playColour.withAlpha (s.state == State::Overdubbing ? 0.5f : 0.8f), false);
            if (owner.overlayUsed)
                drawBars (g, wave, owner.overlay, 1.0f, overdubColour, true);

            if (s.lengthSeconds > 0.0 && s.state != State::Stopped)
            {
                const float x = wave.getX() + wave.getWidth() * (float) juce::jlimit (0.0, 1.0, s.positionSeconds / s.lengthSeconds);
                g.setColour (juce::Colours::white.withAlpha (0.14f));
                g.fillRect (x - 4.0f, wave.getY(), 8.0f, wave.getHeight());
                g.setColour (juce::Colours::white.withAlpha (0.95f));
                g.fillRect (x - 1.0f, wave.getY() - 3.0f, 2.0f, wave.getHeight() + 6.0f);
            }
        }

        //==============================================================================
        // bottom line: layers, tempo, what is switched on, input level
        {
            // input level
            auto meter = bottom.removeFromRight (118.0f);
            g.setFont (AmpLookAndFeel::font (10.5f, true).withExtraKerningFactor (0.2f));
            g.setColour (t.panelTextDim);
            g.drawText ("IN", meter.removeFromLeft (22.0f), juce::Justification::centredLeft, false);

            const auto bar = meter.withSizeKeepingCentre (meter.getWidth(), 6.0f);
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRoundedRectangle (bar, 3.0f);

            const float db = juce::Decibels::gainToDecibels (v.meter, -60.0f);
            const float filled = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            if (filled > 0.0f)
            {
                g.setColour (v.meter >= 1.0f ? recordColour : (db > -6.0f ? overdubColour : playColour));
                g.fillRoundedRectangle (bar.withWidth (juce::jmax (6.0f, bar.getWidth() * filled)), 3.0f);
            }

            bottom.removeFromRight (14.0f);

            // tags, from the right
            const auto tag = [&g, &bottom] (const juce::String& text, Colour c)
            {
                const auto f = AmpLookAndFeel::font (10.5f, true).withExtraKerningFactor (0.14f);
                const float width = juce::GlyphArrangement::getStringWidth (f, text) + 18.0f;
                const auto area = bottom.removeFromRight (width).withSizeKeepingCentre (width, 19.0f);
                bottom.removeFromRight (6.0f);
                g.setColour (c.withAlpha (0.14f));
                g.fillRoundedRectangle (area, 5.0f);
                g.setColour (c.withAlpha (0.9f));
                g.setFont (f);
                g.drawText (text, area, juce::Justification::centred, false);
            };

            if (v.reverse)
                tag ("REVERSE", t.accent);
            if (v.speed != 1)
                tag (v.speed == 0 ? "HALF SPEED" : "DOUBLE SPEED", t.accent);
            if (v.sync)
                tag ("SYNC", t.accent);
            if (v.rhythmOn || s.autoRunning)
                tag (v.rhythmName.toUpperCase(), overdubColour);
            if (v.autoLoop > 0)
                tag ("AUTO", recordColour);

            juce::String info;
            if (hasLoop)
            {
                info << juce::String (s.layers) << (s.layers == 1 ? " layer" : " layers");
                if (s.loopBpm > 0.0)
                {
                    // (a loop played freely is no whole number of beats: only its tempo is shown)
                    const auto bars = musicalLength (s.lengthSeconds * s.loopBpm / 60.0, s.beatsPerBar);
                    info << "     " << (bars.isNotEmpty() ? bars + " at " : juce::String ("recorded at "))
                         << juce::String (s.loopBpm, 1) << " BPM";
                }
            }
            else if (s.state == State::Recording && s.autoRunning && s.targetSeconds > 0.0)
            {
                info << "Recording " << v.autoLength.toLowerCase();
                if (s.autoLayers > 0)
                    info << ", then " << juce::String (s.autoLayers) << (s.autoLayers == 1 ? " more layer" : " more layers");
            }
            else if (s.state == State::Recording && v.lengthName.isNotEmpty() && s.targetSeconds > 0.0)
            {
                info << "Recording " << v.lengthName.toLowerCase();
            }

            g.setFont (AmpLookAndFeel::font (13.5f));
            g.setColour (t.panelText.withAlpha (0.8f));
            g.drawText (info, bottom, juce::Justification::centredLeft, false);
        }
    }

private:
    LooperEditor& owner;
};

//==============================================================================
/** Tempo and the beat, in the header. */
class LooperEditor::TempoReadout : public juce::Component
{
public:
    explicit TempoReadout (LooperEditor& e) : owner (e) { setInterceptsMouseClicks (false, false); }

    void paint (juce::Graphics& g) override
    {
        const auto& v = owner.view;
        const auto& s = v.status;
        const auto& t = owner.lookAndFeel.getTheme();
        auto r = getLocalBounds().toFloat();

        // one dot per counted beat of the bar (two in 6/8, seven in 7/8)
        const double beat = countedBeat (s.numerator, s.denominator);
        const int beats = juce::jlimit (1, 12, juce::roundToInt (s.beatsPerBar / beat));
        const double position = s.beatInBar / beat;
        const int current = juce::jlimit (0, beats - 1, (int) std::floor (position));
        const float fade = 1.0f - 0.65f * (float) (position - std::floor (position));
        auto dots = r.removeFromRight ((float) beats * 15.0f);

        for (int i = 0; i < beats; ++i)
        {
            const auto dot = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ dots.getX() + 7.5f + (float) i * 15.0f, dots.getCentreY() });
            const bool on = v.beatAudible && i == current;
            g.setColour (on ? (i == 0 ? overdubColour : t.accent).withAlpha (fade) : juce::Colours::white.withAlpha (0.13f));
            g.fillEllipse (dot);
        }

        r.removeFromRight (14.0f);
        auto text = r.removeFromRight (190.0f);
        g.setFont (AmpLookAndFeel::font (20.0f, true));
        g.setColour (t.headerText);
        g.drawText (juce::String (s.bpm, 1) + " BPM", text.removeFromTop (26.0f), juce::Justification::centredRight, false);
        g.setFont (AmpLookAndFeel::font (10.0f, true).withExtraKerningFactor (0.22f));
        g.setColour (t.headerText.withAlpha (0.5f));
        g.drawText (juce::String (v.hostTempo ? "HOST TEMPO" : "TEMPO KNOB") + "  /  " + juce::String (s.numerator) + "/"
                        + juce::String (s.denominator),
                    text, juce::Justification::centredRight, false);
    }

private:
    LooperEditor& owner;
};

//==============================================================================
LooperEditor::LooperEditor (LooperProcessor& p)
    : AudioProcessorEditor (p),
      looper (p),
      lookAndFeel (looperTheme()),
      level (p.getParameters(), "level", "Loop Level"),
      feedback (p.getParameters(), "feedback", "Feedback"),
      tempo (p.getParameters(), "tempo", "Tempo"),
      monitor (p.getParameters(), "monitor", "Monitor"),
      reverse (p.getParameters(), "reverse", "Reverse"),
      sync (p.getParameters(), "sync", "Sync"),
      autoStart (p.getParameters(), "autostart", "Auto Start"),
      click (p.getParameters(), "click", "Rhythm"),
      rhythmLevel (p.getParameters(), "rhythmlevel", "Level"),
      rhythmPattern (p.getParameters(), "rhythm", "Pattern"),
      timeSignature (p.getParameters(), "timesig", "Time"),
      autoLoop (p.getParameters(), "autoloop", "Auto Loop"),
      countIn (p.getParameters(), "countin", "Count-in"),
      afterRecord (p.getParameters(), "afterrec", "After Rec"),
      stopMode (p.getParameters(), "stopmode", "Stop"),
      speed (p.getParameters(), "speed", "Speed"),
      quantize (p.getParameters(), "quantize", "Quantize"),
      length (p.getParameters(), "length", "Length"),
      switchMode (p.getParameters(), "switchmode", "Switches"),
      tooltips (this, 700)
{
    setLookAndFeel (&lookAndFeel);
    overlay.assign ((size_t) LooperProcessor::overviewSize, 0.0f);

    content.setSize (baseWidth, baseHeight);
    addAndMakeVisible (content);

    backdrop = std::make_unique<Backdrop> (*this);
    display = std::make_unique<Display> (*this);
    mainButton = std::make_unique<MainButton> (*this);
    tempoReadout = std::make_unique<TempoReadout> (*this);

    content.addAndMakeVisible (*backdrop);
    content.addAndMakeVisible (*display);
    content.addAndMakeVisible (*mainButton);
    content.addAndMakeVisible (*tempoReadout);

    backdrop->setBounds (content.getLocalBounds());
    display->setBounds (displayArea);
    tempoReadout->setBounds (baseWidth - 20 - 380, 10, 380, 44);

    //==============================================================================
    // left: transport
    const int left = leftPanelArea.getX(), panelWidth = leftPanelArea.getWidth();
    mainButton->setBounds (left + (panelWidth - 220) / 2, 84, 220, 220);

    hint.setFont (AmpLookAndFeel::font (13.5f));
    hint.setJustificationType (juce::Justification::centred);
    hint.setMinimumHorizontalScale (0.8f);
    hint.setInterceptsMouseClicks (false, false);
    hint.setBounds (left + 8, 310, panelWidth - 16, 46);
    content.addAndMakeVisible (hint);

    // The stop button is momentary like the main one: holding it clears the loop.
    stopButton.setTooltip ("Stop the loop (the Stop setting decides how). Click again to stop at once, hold to clear.");
    stopButton.onStateChange = [this]
    {
        const bool down = stopButton.isDown();
        if (down != stopIsDown)
        {
            stopIsDown = down;
            looper.sendSwitch (down ? LooperProcessor::Switch::stopDown : LooperProcessor::Switch::stopUp);
        }
    };

    undoButton.setTooltip ("Take back the last overdub, or bring it back. While recording: cancel.");
    undoButton.onClick = [this] { looper.sendSwitch (LooperProcessor::Switch::undo); };

    clearButton.setTooltip ("Erase the loop. Click twice: this can't be undone.");
    clearButton.onClick = [this]
    {
        const auto now = juce::Time::getMillisecondCounter();
        if (clearArmedAt != 0 && now - clearArmedAt < 2500u)
        {
            clearArmedAt = 0;
            looper.sendSwitch (LooperProcessor::Switch::clear);
        }
        else
        {
            clearArmedAt = std::max<juce::uint32> (1u, now);
        }
        updateControls();
    };

    doubleButton.setButtonText (utf8 ("\xc3\x97" "2"));
    doubleButton.setTooltip ("Make the loop twice as long by repeating it - room for a longer overdub");
    doubleButton.onClick = [this]
    {
        const auto outcome = looper.doubleLoop();
        showMessage (outcome.message, ! outcome.ok);
    };

    halveButton.setButtonText (utf8 ("\xc3\xb7" "2"));
    halveButton.setTooltip ("Keep only the first half of the loop");
    halveButton.onClick = [this]
    {
        const auto outcome = looper.halveLoop();
        showMessage (outcome.message, ! outcome.ok);
    };

    saveButton.setTooltip ("Save the loop as a WAV file (the loop is also saved with your project)");
    saveButton.onClick = [this] { chooseFile (true); };
    loadButton.setTooltip ("Load an audio file as the loop (you can also drop a file onto the window)");
    loadButton.onClick = [this] { chooseFile (false); };

    {
        const int w = 82, gap = 4, x = left + (panelWidth - (3 * w + 2 * gap)) / 2;
        int i = 0;
        for (auto* b : { &stopButton, &undoButton, &clearButton })
        {
            b->setBounds (x + i++ * (w + gap), 364, w, 44);
            content.addAndMakeVisible (*b);
        }
    }
    {
        const int w = 59, gap = 6, x = left + (panelWidth - (4 * w + 3 * gap)) / 2;
        int i = 0;
        for (auto* b : { &doubleButton, &halveButton, &saveButton, &loadButton })
        {
            b->setBounds (x + i++ * (w + gap), 416, w, 32);
            content.addAndMakeVisible (*b);
        }
    }

    // Auto loop: one click, and the looper counts in, records and layers by itself.
    groups.push_back ({ "AUTO LOOP", { left + 14, 462, panelWidth - 28, 16 } });
    autoLoop.setBounds (left + 18, 482, 136, 62);
    countIn.setBounds (left + panelWidth - 18 - 112, 482, 112, 62);
    autoLoop.setMaxBoxWidth (128);
    autoLoop.box.setTooltip ("One click does it all: a count-in, then the recording (as long as Length says, or 4 bars), "
                             "then more layers one loop cycle each if you ask for them - and the loop plays. "
                             "The rhythm keeps time until then. Click any time to take over.");
    countIn.box.setTooltip ("How many bars the auto loop counts in before it records");
    content.addAndMakeVisible (autoLoop);
    content.addAndMakeVisible (countIn);

    groups.push_back ({ "LEVELS", { left + 14, 562, panelWidth - 28, 16 } });
    level.setBounds (left + 30, 584, 106, 132);
    feedback.setBounds (left + panelWidth - 30 - 106, 584, 106, 132);
    level.slider.setTooltip ("How loud the loop plays");
    feedback.slider.setTooltip ("How much of the loop survives each overdub pass. Below 100 % old layers fade away while you add new ones.");
    content.addAndMakeVisible (level);
    content.addAndMakeVisible (feedback);

    //==============================================================================
    // right: settings in two rows
    const auto row = [this] (int y, const std::vector<RowGroup>& rowGroups)
    {
        const int x0 = controlsArea.getX() + 18, available = controlsArea.getWidth() - 36;

        int used = 0, spaces = -1;
        for (const auto& group : rowGroups)
        {
            ++spaces;
            for (const auto& control : group.controls)
                used += control.second;
        }

        const int gap = spaces > 0 ? (available - used) / spaces : 0;
        int x = x0 + (spaces > 0 ? 0 : (available - used) / 2);

        for (const auto& group : rowGroups)
        {
            const int start = x;
            for (const auto& control : group.controls)
            {
                control.first->setBounds (x, y + 20, control.second, 108);
                content.addAndMakeVisible (*control.first);
                x += control.second;
            }
            groups.push_back ({ group.caption, { start, y, x - start, 16 } });
            x += gap;
        }
    };

    row (324, { { "INPUT",     { { &monitor, 92 } } },
                { "TRANSPORT", { { &afterRecord, 118 }, { &stopMode, 118 } } },
                { "PLAYBACK",  { { &speed, 108 }, { &reverse, 84 } } },
                { "CONTROLLER", { { &switchMode, 122 } } } });

    row (462, { { "TEMPO", { { &sync, 96 }, { &quantize, 124 }, { &length, 124 }, { &autoStart, 104 }, { &tempo, 112 } } } });

    row (600, { { "GUIDE RHYTHM", { { &click, 96 }, { &rhythmPattern, 164 }, { &timeSignature, 112 }, { &rhythmLevel, 112 } } } });
    rhythmPattern.setMaxBoxWidth (150);

    monitor.button.setTooltip ("Pass your playing through to the output. Switch it off if you already hear yourself another way.");
    afterRecord.box.setTooltip ("What happens when you close the first recording: play the loop, or keep recording on top of it");
    stopMode.box.setTooltip ("How the loop stops: at once, with a fade, or at the end of the loop");
    speed.box.setTooltip ("Half speed (an octave down, twice as long), normal or double speed. Overdubs follow.");
    reverse.button.setTooltip ("Play the loop backwards");
    switchMode.box.setTooltip ("For a foot controller mapped to the Main/Stop/Undo/Clear Switch parameters in your DAW: "
                               "Momentary = the parameter is on while the switch is held, Latching = it flips with every press");
    sync.button.setTooltip ("Work with the tempo: start and stop on the beat or bar while the DAW plays (or the click is on), "
                            "record fixed lengths, and follow tempo changes");
    quantize.box.setTooltip ("With Sync on, and while the DAW plays or the click is on: recording and playback start and stop on the next beat or bar");
    length.box.setTooltip ("With Sync on: record exactly this long, then carry straight on (Free = until you click again)");
    autoStart.button.setTooltip ("Recording waits for your first note: click, get ready, play");
    click.button.setTooltip ("Plays the rhythm all the time, in the looper's output (it is never recorded into the loop). "
                             "Without it the rhythm only plays through an auto loop.");
    rhythmPattern.box.setTooltip ("A click (on the beats, or with eighths, triplets, sixteenths or a shuffle) or a simple drum groove");
    timeSignature.box.setTooltip ("Used when the host doesn't give one (the standalone app)");
    rhythmLevel.slider.setTooltip ("How loud the rhythm plays");
    tempo.slider.setTooltip ("The looper's own tempo, used when the host doesn't give one (the standalone app)");

    sendLookAndFeelChange();
    updateView();
    updateControls();

    const double savedScale = (double) looper.getParameters().state.getProperty (LooperProcessor::uiScaleProperty, 1.0);
    const double scale = juce::jlimit (0.5, 2.0, savedScale);
    setResizable (true, true);
    setResizeLimits ((int) (baseWidth * 0.5), (int) (baseHeight * 0.5), baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (juce::roundToInt (baseWidth * scale), juce::roundToInt (baseHeight * scale));

    startTimerHz (30);
}

LooperEditor::~LooperEditor()
{
    stopTimer();
    if (stopIsDown)
        looper.sendSwitch (LooperProcessor::Switch::stopUp);
    setLookAndFeel (nullptr);
}

void LooperEditor::paint (juce::Graphics& g)
{
    g.fillAll (lookAndFeel.getTheme().background);
}

void LooperEditor::resized()
{
    const float scale = (float) getWidth() / (float) baseWidth;
    content.setBounds (0, 0, baseWidth, baseHeight);
    content.setTransform (juce::AffineTransform::scale (scale));
    looper.getParameters().state.setProperty (LooperProcessor::uiScaleProperty, scale, nullptr);
}

float LooperEditor::parameter (const char* id) const
{
    if (const auto* value = looper.getParameters().getRawParameterValue (id))
        return value->load();
    return 0.0f;
}

//==============================================================================
void LooperEditor::refresh()
{
    const auto before = view;
    updateView();
    updateControls();

    const auto& a = before.status;
    const auto& b = view.status;
    const bool moving = b.state == State::Armed || b.state == State::Recording || b.state == State::Playing
                     || b.state == State::Overdubbing || b.scheduled || b.stopping;
    const bool changed = a.state != b.state || a.scheduled != b.scheduled || a.stopping != b.stopping
                      || a.layers != b.layers || a.generation != b.generation
                      || std::abs (a.lengthSeconds - b.lengthSeconds) > 1.0e-9 || std::abs (a.loopBpm - b.loopBpm) > 1.0e-9
                      || before.sync != view.sync || before.autoStart != view.autoStart || before.reverse != view.reverse
                      || before.speed != view.speed || before.quantize != view.quantize
                      || before.overdubAfterRecord != view.overdubAfterRecord || before.lengthName != view.lengthName
                      || before.autoLoop != view.autoLoop || before.autoLength != view.autoLength
                      || before.rhythmOn != view.rhythmOn || before.rhythmName != view.rhythmName
                      || a.autoRunning != b.autoRunning || a.autoLayer != b.autoLayer
                      || std::abs (before.meter - view.meter) > 1.0e-4f;

    if (moving || changed || mainButton->isMouseButtonDown())
    {
        display->repaint();
        mainButton->repaint();
    }

    if (view.beatAudible || before.beatAudible || std::abs (a.bpm - b.bpm) > 1.0e-6
        || std::abs (a.beatsPerBar - b.beatsPerBar) > 1.0e-9 || before.hostTempo != view.hostTempo
        || a.numerator != b.numerator || a.denominator != b.denominator)
        tempoReadout->repaint();
}

void LooperEditor::updateView()
{
    const auto previous = view.status.state;
    const float input = looper.takeInputPeak();

    View v;
    v.status = looper.getStatus();
    v.sync = parameter ("sync") > 0.5f;
    v.autoStart = parameter ("autostart") > 0.5f;
    v.click = parameter ("click") > 0.5f;
    v.reverse = parameter ("reverse") > 0.5f;
    v.overdubAfterRecord = parameter ("afterrec") > 0.5f;
    v.speed = juce::roundToInt (parameter ("speed"));
    v.quantize = juce::roundToInt (parameter ("quantize"));
    v.hostTempo = looper.isUsingHostTempo();
    v.hostMeter = looper.isUsingHostTimeSignature();
    v.quantising = v.sync && v.quantize != 0 && (looper.isHostPlaying() || v.click);
    v.beatAudible = looper.isHostPlaying() || v.click || v.status.autoRunning;

    v.autoLoop = juce::jlimit (0, 4, juce::roundToInt (parameter ("autoloop")));
    v.rhythmOn = v.click;
    v.rhythmName = looper::RhythmGuide::patternName (juce::roundToInt (parameter ("rhythm")));

    const int lengthIndex = juce::jlimit (0, 7, juce::roundToInt (parameter ("length")));
    if ((v.sync || v.autoLoop > 0) && lengthIndex > 0)
        v.lengthName = lengthNames[lengthIndex];
    v.autoLength = lengthIndex > 0 ? juce::String (lengthNames[lengthIndex]) : juce::String ("4 Bars");

    v.meter = juce::jmax (input, view.meter * 0.8f);
    if (v.meter < 1.0e-4f)
        v.meter = 0.0f;

    view = v;
    const auto& s = view.status;
    const int bins = LooperProcessor::overviewSize;

    // a new picture of the loop
    if (const auto serial = looper.getOverviewSerial(); serial != seenOverviewSerial)
    {
        seenOverviewSerial = serial;
        looper.copyOverview (peaks, peaksLength, peaksGeneration);
        std::fill (overlay.begin(), overlay.end(), 0.0f);
        overlayUsed = false;
    }

    // while recording: the input level over time
    if (s.state == State::Recording)
    {
        if (previous != State::Recording)
            live.clear();
        live.push_back (input);
    }
    else if (s.state == State::Empty || s.state == State::Armed)
    {
        live.clear();
    }

    // while overdubbing: what is being added, where the play position passes
    if (s.state == State::Overdubbing && s.lengthSeconds > 0.0)
    {
        const int bin = juce::jlimit (0, bins - 1, (int) (s.positionSeconds / s.lengthSeconds * (double) bins));
        if (lastOverlayBin < 0)
            lastOverlayBin = bin;

        int distance = bin - lastOverlayBin; // the short way round, so reverse and the loop point work
        if (distance > bins / 2)
            distance -= bins;
        else if (distance < -bins / 2)
            distance += bins;

        const int direction = distance >= 0 ? 1 : -1;
        for (int i = 0, b = lastOverlayBin; i <= std::abs (distance); ++i, b = (b + direction + bins) % bins)
            overlay[(size_t) b] = juce::jmax (overlay[(size_t) b], input);

        lastOverlayBin = bin;
        overlayUsed = true;
    }
    else
    {
        lastOverlayBin = -1;
    }
}

void LooperEditor::updateControls()
{
    const auto& s = view.status;
    const auto now = juce::Time::getMillisecondCounter();
    const bool settled = s.state == State::Playing || s.state == State::Stopped;

    stopButton.setEnabled (s.state != State::Empty);
    undoButton.setButtonText (s.canRedo ? "REDO" : "UNDO");
    undoButton.setEnabled (s.canUndo || s.canRedo || s.state == State::Armed || s.state == State::Recording
                           || s.state == State::Overdubbing);

    if (s.state == State::Empty || (clearArmedAt != 0 && now - clearArmedAt >= 2500u))
        clearArmedAt = 0;
    clearButton.setEnabled (s.state != State::Empty);
    clearButton.setButtonText (clearArmedAt != 0 ? "SURE?" : "CLEAR");
    clearButton.setColour (juce::TextButton::buttonColourId, clearArmedAt != 0 ? Colour (0xff8c2327) : Colour (0xff2a2b2f));

    doubleButton.setEnabled (settled);
    halveButton.setEnabled (settled);
    saveButton.setEnabled (settled);

    // Controls that do nothing in the current setup are greyed out: Quantize without Sync, Length
    // without Sync or an auto loop, Count-in without an auto loop, Tempo and Time with the host's.
    const auto enable = [] (juce::Component& c, bool on)
    {
        c.setEnabled (on);
        c.setAlpha (on ? 1.0f : 0.4f);
    };
    enable (quantize, view.sync);
    enable (length, view.sync || view.autoLoop > 0);
    enable (countIn, view.autoLoop > 0);
    enable (tempo, ! view.hostTempo);
    enable (timeSignature, ! view.hostMeter);

    const bool showingMessage = message.isNotEmpty() && now - messageAt < 4500u;
    if (! showingMessage)
        message.clear();

    hint.setColour (juce::Label::textColourId,
                    showingMessage ? (messageIsError ? Colour (0xffff8a80) : lookAndFeel.getTheme().accent.brighter (0.3f))
                                   : lookAndFeel.getTheme().panelText.withAlpha (0.82f));
    hint.setText (showingMessage ? message : hintText(), juce::dontSendNotification);
}

juce::String LooperEditor::hintText() const
{
    const auto& s = view.status;
    const juce::String unit = view.quantize == 1 ? "beat" : "bar";

    // auto loop
    if (s.state == State::Empty && view.autoLoop > 0)
        return "Click once: the looper counts in,\nrecords " + view.autoLength.toLowerCase()
             + (view.autoLoop > 1 ? " and " + juce::String (view.autoLoop - 1) + (view.autoLoop == 2 ? " more layer" : " more layers")
                                  : juce::String())
             + ", then plays";
    if (s.autoRunning)
    {
        if (s.state == State::Armed)
            return "Get ready to play...\nClick to cancel";
        if (s.state == State::Recording)
            return "Recording " + view.autoLength.toLowerCase() + "\nClick to take over";
        if (s.state == State::Overdubbing)
            return "Layer " + juce::String (s.autoLayer + 1) + " of " + juce::String (s.autoLayers + 1)
                 + ": play something new\nClick to take over";
    }

    switch (s.state)
    {
        case State::Empty:
            if (view.autoStart)
                return "Click, then play:\nrecording starts with your first note";
            if (view.quantising)
                return "Click to record from the next " + unit;
            if (view.lengthName.isNotEmpty())
                return "Click to record " + view.lengthName.toLowerCase() + ".\nThe loop then carries on by itself";
            return "Click to record.\nClick again to close the loop";

        case State::Armed:
            return view.autoStart ? "Waiting for your first note...\nClick to cancel"
                                  : "Recording starts on the next " + unit + "...\nClick to cancel";

        case State::Recording:
            if (s.targetSeconds > 0.0)
                return "Recording " + view.lengthName.toLowerCase() + ":\nthe loop closes by itself";
            if (s.scheduled)
                return "The loop closes on the next " + unit;
            return "Click to close the loop.\nUndo cancels the recording";

        case State::Playing:
            if (s.stopping)
                return "Stopping...\nClick to keep playing";
            return juce::String ("Click: overdub   Double-click: stop\nHold: ") + (s.canRedo ? "redo" : "undo") + " the last overdub";

        case State::Overdubbing:
            return "Click: finish the overdub\nHold: take it back";

        case State::Stopped:
            if (s.scheduled)
                return "Starts on the next " + unit + "...\nClick to cancel";
            return "Click: play\nHold: clear the loop";

        default:
            break;
    }

    return {};
}

void LooperEditor::showMessage (const juce::String& text, bool isError)
{
    message = text;
    messageIsError = isError;
    messageAt = juce::Time::getMillisecondCounter();
    updateControls();
}

//==============================================================================
void LooperEditor::chooseFile (bool save)
{
    static juce::File lastFolder; // remembered while the plugin is loaded

    auto start = lastFolder.isDirectory() ? lastFolder : juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    if (save)
        start = start.getChildFile ("Loop.wav");

    chooser = std::make_unique<juce::FileChooser> (save ? "Save the loop as a WAV file" : "Load an audio file as the loop",
                                                   start, save ? juce::String ("*.wav") : audioWildcard());

    const int chooserFlags = save ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting
                           : juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    juce::Component::SafePointer<LooperEditor> safeThis (this);
    chooser->launchAsync (chooserFlags, [safeThis, save] (const juce::FileChooser& fc)
    {
        if (safeThis == nullptr)
            return;

        const auto file = fc.getResult();
        if (file == juce::File())
            return;

        lastFolder = file.getParentDirectory();

        if (save)
        {
            const auto outcome = safeThis->looper.saveLoopToFile (file);
            safeThis->showMessage (outcome.message, ! outcome.ok);
        }
        else
        {
            safeThis->loadFile (file);
        }
    });
}

void LooperEditor::loadFile (const juce::File& file)
{
    const auto outcome = looper.loadLoopFromFile (file);
    showMessage (outcome.message, ! outcome.ok);
}

bool LooperEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    return files.size() == 1 && juce::WildcardFileFilter (audioWildcard(), {}, {}).isFileSuitable (juce::File (files[0]));
}

void LooperEditor::filesDropped (const juce::StringArray& files, int, int)
{
    if (files.size() == 1)
        loadFile (juce::File (files[0]));
}

} // namespace oap::ui
