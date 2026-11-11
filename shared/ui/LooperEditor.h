#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "plugin/LooperProcessor.h"
#include "ui/AmpControls.h"
#include "ui/AmpLookAndFeel.h"

namespace oap::ui
{

//==============================================================================
/** The Looper's window, made for the mouse: a big transport button (click,
    double-click and hold work like a looper's footswitch), a display with the
    loop's waveform, play position and layers, and the settings in groups.
    Audio files can be dropped onto it. */
class LooperEditor : public juce::AudioProcessorEditor,
                     public juce::FileDragAndDropTarget,
                     private juce::Timer
{
public:
    explicit LooperEditor (LooperProcessor&);
    ~LooperEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    /** Brings everything on screen up to date. A timer does this 30 times a second. */
    void refresh();

    static constexpr int baseWidth = 1040, baseHeight = 760;

private:
    class Backdrop;
    class MainButton;
    class Display;
    class TempoReadout;

    /** Everything the painted parts show, gathered once per timer tick. */
    struct View
    {
        looper::Status status;
        bool sync = false, autoStart = false, click = false, reverse = false, overdubAfterRecord = false;
        bool quantising = false;   // actions wait for the next beat or bar
        bool beatAudible = false;  // the host is playing or the click is on
        bool hostTempo = false;
        int speed = 1;             // 0 half, 1 normal, 2 double
        int quantize = 2;          // 0 off, 1 beat, 2 bar
        juce::String lengthName;   // "2 Bars", empty = free
        float meter = 0.0f;
        int autoLoop = 0;          // 0 off, else how many layers the auto loop records
        juce::String autoLength;   // how long an auto loop records ("4 Bars")
        bool rhythmOn = false, hostMeter = false;
        juce::String rhythmName;
    };

    struct Group
    {
        juce::String caption;
        juce::Rectangle<int> bounds;
    };

    void timerCallback() override { refresh(); }
    void updateView();
    void updateControls();
    juce::String hintText() const;
    void showMessage (const juce::String& text, bool isError);
    void chooseFile (bool save);
    void loadFile (const juce::File&);
    float parameter (const char* id) const;

    LooperProcessor& looper;
    AmpLookAndFeel lookAndFeel;
    juce::Component content;

    std::unique_ptr<Backdrop> backdrop;
    std::unique_ptr<MainButton> mainButton;
    std::unique_ptr<Display> display;
    std::unique_ptr<TempoReadout> tempoReadout;

    juce::TextButton stopButton { "STOP" }, undoButton { "UNDO" }, clearButton { "CLEAR" };
    juce::TextButton doubleButton, halveButton, saveButton { "SAVE" }, loadButton { "LOAD" };
    juce::Label hint;

    AmpKnob level, feedback, tempo;
    AmpSwitch monitor, reverse, sync, autoStart, click;
    AmpKnob rhythmLevel;
    AmpChoice rhythmPattern, timeSignature, autoLoop, countIn;
    AmpChoice afterRecord, stopMode, speed, quantize, length, switchMode;
    std::vector<Group> groups;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::TooltipWindow tooltips;

    View view;
    std::vector<float> peaks, overlay, live;
    double peaksLength = 0.0;
    std::uint32_t peaksGeneration = 0, seenOverviewSerial = 0;
    bool overlayUsed = false;
    int lastOverlayBin = -1;

    bool stopIsDown = false;
    juce::uint32 clearArmedAt = 0, messageAt = 0;
    juce::String message;
    bool messageIsError = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperEditor)
};

} // namespace oap::ui
