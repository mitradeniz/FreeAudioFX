#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "looper/LooperEngine.h"

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace oap
{

//==============================================================================
/** The Looper plugin: wraps looper::LooperEngine with parameters, host tempo,
    switch events from the editor and from mapped parameters, and everything
    that must not happen on the audio thread:

    - a background thread keeps loop memory allocated ahead of the recording
      and, whenever the loop changes, takes a copy of it for the waveform
      display and compresses it (FLAC), so that the loop is saved with the
      host's project without making the host wait;
    - saving and loading audio files, and converting the loop when the sample
      rate changes. */
class LooperProcessor : public juce::AudioProcessor,
                        private juce::Timer
{
public:
    static constexpr const char* uiScaleProperty = "uiScale";
    static constexpr int overviewSize = 720;

    /** Switch events from the editor. */
    enum class Switch { mainDown, mainUp, stopDown, stopUp, undo, clear };

    /** Result of an action started from the editor. */
    struct Outcome
    {
        bool ok = true;
        juce::String message; // why it failed, or a note about what happened
    };

    LooperProcessor();
    ~LooperProcessor() override;

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    //==============================================================================
    // For the editor (message thread)

    /** Presses or releases one of the switches. Reaches the audio thread with the next block. */
    void sendSwitch (Switch) noexcept;

    looper::Status getStatus() const noexcept { return engine.getStatus(); }

    /** True when the tempo comes from the host, false when the Tempo control is used. */
    bool isUsingHostTempo() const noexcept { return hostTempo.load(); }

    /** True while the host's transport is running. */
    bool isHostPlaying() const noexcept { return hostPlaying.load(); }

    /** True when the time signature comes from the host, false when the Time Signature control is used. */
    bool isUsingHostTimeSignature() const noexcept { return hostMeter.load(); }

    /** Highest input level since the last call (linear). */
    float takeInputPeak() noexcept { return inputPeak.exchange (0.0f); }

    /** Changes whenever a new picture of the loop is available. */
    std::uint32_t getOverviewSerial() const noexcept { return overviewSerial.load(); }

    /** The picture of the loop: `overviewSize` peak levels, empty when there is no loop. */
    void copyOverview (std::vector<float>& peaks, double& lengthSeconds, std::uint32_t& generation);

    Outcome saveLoopToFile (const juce::File&);
    Outcome loadLoopFromFile (const juce::File&);
    Outcome doubleLoop();
    Outcome halveLoop();

    //==============================================================================
    const juce::String getName() const override { return "OAP Looper"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override { prepared = false; }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return {}; }
    void changeProgramName (int, const juce::String&) override  {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

private:
    /** A loop outside the engine. */
    struct LoopData
    {
        std::vector<float> left, right;
        double sampleRate = 0.0, bpm = 0.0;
    };

    /** The last copy taken of the loop, ready to be saved or drawn. */
    struct Snapshot
    {
        bool valid = false;
        std::uint32_t generation = 0, loopId = 0;
        int numSamples = 0, channels = 0;
        double sampleRate = 0.0, bpm = 0.0;
        float scale = 1.0f;          // the audio was divided by this to fit into the file
        juce::MemoryBlock audio;     // FLAC, empty when there is no loop
        std::vector<float> peaks;
    };

    struct QueuedSwitch
    {
        Switch type = Switch::mainUp;
        std::uint32_t time = 0;
    };

    class Worker;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void timerCallback() override;
    void collectSwitches() noexcept;
    void applySwitch (Switch) noexcept;
    looper::Settings readSettings() const noexcept;
    looper::Timing readTiming() noexcept;

    /** Brings the snapshot up to date with the engine. False when that isn't possible right now:
        the loop is being recorded, or (with wait = false) another thread is already at it. */
    bool refreshSnapshot (bool wait);
    void workerLoop (juce::Thread&);

    /** Puts a loop into the engine (resampled if necessary). Empty data clears the loop.
        The caller holds refreshMutex. */
    looper::LooperEngine::Result installLoop (LoopData&, bool audioStopped);

    static bool encodeLoop (const std::vector<float>& left, const std::vector<float>& right, Snapshot&);
    static bool decodeLoop (const void* data, size_t size, int numSamples, float scale, LoopData&);

    //==============================================================================
    looper::LooperEngine engine;
    juce::AudioProcessorValueTreeState parameters;

    struct Params
    {
        std::atomic<float>* level = nullptr;
        std::atomic<float>* feedback = nullptr;
        std::atomic<float>* monitor = nullptr;
        std::atomic<float>* afterRecord = nullptr;
        std::atomic<float>* stopMode = nullptr;
        std::atomic<float>* speed = nullptr;
        std::atomic<float>* reverse = nullptr;
        std::atomic<float>* sync = nullptr;
        std::atomic<float>* quantize = nullptr;
        std::atomic<float>* length = nullptr;
        std::atomic<float>* autoStart = nullptr;
        std::atomic<float>* click = nullptr;
        std::atomic<float>* rhythm = nullptr;
        std::atomic<float>* rhythmLevel = nullptr;
        std::atomic<float>* timeSig = nullptr;
        std::atomic<float>* autoLoop = nullptr;
        std::atomic<float>* countIn = nullptr;
        std::atomic<float>* tempo = nullptr;
        std::atomic<float>* switchMode = nullptr;
        std::atomic<float>* mainSwitch = nullptr;
        std::atomic<float>* stopSwitch = nullptr;
        std::atomic<float>* undoSwitch = nullptr;
        std::atomic<float>* clearSwitch = nullptr;
    } params;

    // editor -> audio thread
    juce::AbstractFifo switchFifo { 64 };
    std::array<QueuedSwitch, 64> switchQueue {};

    // audio thread
    std::array<Switch, 96> waiting {};   // switch events held back while another thread uses the loop
    int numWaiting = 0;
    bool lastMain = false, lastStop = false, lastUndo = false, lastClear = false;
    std::atomic<bool> resyncSwitches { true }, cancelRecording { false };
    juce::AudioBuffer<float> scratch;

    std::atomic<bool> hostTempo { false }, hostMeter { false }, hostPlaying { false }, prepared { false };
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<double> currentRate { 48000.0 };

    // loop snapshots
    std::mutex refreshMutex;             // one refresh, loop replacement or sample-rate change at a time
    std::mutex snapshotMutex;            // guards `snapshot`, held only briefly
    Snapshot snapshot;
    std::vector<float> copyLeft, copyRight;
    std::atomic<std::uint32_t> overviewSerial { 0 };

    std::unique_ptr<Worker> worker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperProcessor)
};

} // namespace oap
