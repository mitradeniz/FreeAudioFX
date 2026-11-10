#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace oap::looper
{

//==============================================================================
enum class State { Empty, Armed, Recording, Playing, Overdubbing, Stopped };
enum class StopMode { Instant, FadeOut, LoopEnd };
enum class Quantize { Off, Beat, Bar };

/** Controls, read from the plugin parameters once per block. */
struct Settings
{
    float level = 1.0f;              // loop playback gain
    float feedback = 1.0f;           // 0..1: how much of the old loop survives each overdub pass
    bool monitor = true;             // pass the dry input through
    bool overdubAfterRecord = false; // record -> overdub -> play instead of record -> play -> overdub
    StopMode stopMode = StopMode::Instant;
    float speed = 1.0f;              // 0.5, 1 or 2
    bool reverse = false;

    bool sync = false;               // use the tempo grid: quantised actions, fixed lengths, follow tempo changes
    Quantize quantize = Quantize::Bar;
    double fixedBeats = 0.0;         // fixed recording length in beats ...
    double fixedBars = 0.0;          // ... or in bars (0 and 0 = free length)
    bool autoStart = false;          // recording starts with the first note
    bool click = false;              // metronome
};

/** Tempo information for the current block. */
struct Timing
{
    double bpm = 120.0;
    double beatsPerBar = 4.0;        // in quarter notes
    bool hostPlaying = false;        // when false the looper runs its own beat clock
    double hostBeat = 0.0;           // quarter-note position at the start of the block
    double hostBarStart = 0.0;       // quarter-note position of the last bar line
};

/** What the user interface needs to know. Safe to read from any thread. */
struct Status
{
    State state = State::Empty;
    bool scheduled = false;          // an action is waiting for the next beat or bar
    bool stopping = false;           // fading out or playing to the loop end
    bool canUndo = false, canRedo = false;
    int layers = 0;                  // audible layers (first recording = 1)
    double lengthSeconds = 0.0;      // loop length (or time recorded so far)
    double positionSeconds = 0.0;
    double targetSeconds = 0.0;      // fixed recording length, 0 = free
    double bpm = 120.0;
    double loopBpm = 0.0;            // tempo the loop was recorded at, 0 = unknown
    double beatInBar = 0.0;
    double beatsPerBar = 4.0;
    std::uint32_t generation = 0;    // changes whenever the loop's content changes
};

//==============================================================================
/** A single-track looper: record, play, overdub with undo/redo, stop modes,
    half/double speed and reverse, overdub feedback, tempo-synced recording
    (quantised start/stop, fixed lengths, tempo following), auto start and a
    metronome.

    Real-time rules: process() and the switch functions never allocate, lock
    or wait. Loop memory grows in blocks that another thread hands over
    (serviceMemory()), and the copy/load/resize functions talk to the audio
    thread through a small lock-free handshake, so the loop can be saved,
    replaced or resized while audio is running - and also while it isn't,
    or while it starts and stops.

    Plain C++17 with no JUCE dependency, so the tests can drive it directly. */
class LooperEngine
{
public:
    static constexpr double maxSeconds = 300.0;

    LooperEngine();
    ~LooperEngine();

    /** Call while audio is not running. Keeps the loop when the sample rate hasn't changed. */
    void prepare (double sampleRate);
    double getSampleRate() const noexcept { return sampleRate; }

    //==============================================================================
    // Audio thread

    /** Call first in every audio block: takes the current controls and tempo, and answers
        requests from other threads. Then send the block's switch events, then process(). */
    void beginBlock (const Settings&, const Timing&) noexcept;

    /** False while another thread is reading or replacing the loop; hold on to switch events until it is true again. */
    bool acceptsEvents() const noexcept { return heldKind == 0; }

    /** Asks a thread that is copying the loop to give up, because an event is waiting. */
    void requestAbort() noexcept { abortRequested.store (true); }

    /** The main switch: record -> play -> overdub. Double tap = stop; hold = undo/redo (clear when stopped). */
    void mainSwitch (bool down) noexcept;

    /** Stop switch: tap = stop, hold = clear. */
    void stopSwitch (bool down) noexcept;

    /** Undo or redo the last overdub (cancels an overdub or a recording in progress). */
    void undoSwitch() noexcept;

    void clear() noexcept;

    /** In-place stereo processing. May be called several times per block. */
    void process (float* left, float* right, int numSamples) noexcept;

    //==============================================================================
    // Any thread
    Status getStatus() const noexcept;
    std::uint32_t getGeneration() const noexcept { return generation.load(); }

    /** Changes whenever the loop is replaced by another one: cleared, recorded anew or loaded.
        Overdubs, undo/redo and length changes keep it. */
    std::uint32_t getLoopId() const noexcept { return loopId.load(); }

    //==============================================================================
    // Other threads (never the audio thread). Pass audioStopped = true only
    // when the caller knows process() is not being called (e.g. prepareToPlay).
    enum class Result { ok, empty, busy, tooLong };

    /** Keeps enough loop memory allocated ahead of the recording. Call regularly (e.g. 20 times a second). */
    void serviceMemory();

    /** Copies what the loop currently sounds like. `busy` = being recorded right now, try again later. */
    Result copyLoop (std::vector<float>& left, std::vector<float>& right, double& loopBpm,
                     std::uint32_t& generationOfCopy, bool audioStopped = false);

    /** Replaces the loop (numSamples = 0 clears it). Playback stops; the new loop is ready to play. */
    Result loadLoop (const float* left, const float* right, int numSamples, double loopBpm, bool audioStopped = false);

    /** Doubles the loop by repeating it, or keeps only its first half. Both keep the current layers as they sound. */
    Result doubleLength (bool audioStopped = false);
    Result halveLength (bool audioStopped = false);

private:
    enum class Scheduled { None, StartRecord, CloseRecord, StartPlay };
    enum class PressAction { None, StartedRecord, ClosedRecord, StartedOverdub, EndedOverdub, StartedPlay };
    struct PendingWrite { int index; float left, right, feedback; };
    struct Command { int type = 0; int length = 0; double bpm = 0.0; };

    // --- audio thread
    bool enterAudio() noexcept;
    void leaveAudio() noexcept;
    void serviceHold() noexcept;
    bool isWriting() const noexcept;
    void executeCommand() noexcept;
    void updateTiming (const Timing&) noexcept;
    bool quantizeActive() const noexcept;

    void startRecording (bool withPreRoll) noexcept;
    void closeLoop() noexcept;
    void startPlayback() noexcept;
    void startOverdub() noexcept;
    void endOverdub() noexcept;
    void beginStop() noexcept;
    void cancelStop() noexcept;
    void finishStop() noexcept;
    void requestClear() noexcept;
    void doClear() noexcept;
    void toggleUndo() noexcept;
    void undoAction() noexcept;
    void mainPressed() noexcept;
    void stopPressed() noexcept;
    void onMainHold() noexcept;
    void fireScheduled() noexcept;

    void startSession() noexcept;
    void commitLayer() noexcept;
    void commitPending() noexcept;
    void discardPending() noexcept;
    void writeOverdub (int index, float l, float r, float feedback) noexcept;
    void prepareChunkForWrite (int chunk) noexcept;
    void resolveChunk (int chunk) noexcept;
    void copyChunk (int chunk, bool undoToLoop) noexcept;
    float readAudible (int channel, int index) const noexcept;
    void sweep() noexcept;
    void publishStatus() noexcept;
    void bumpGeneration() noexcept { generation.fetch_add (1); }

    // --- other threads
    bool lockOutAudio();
    bool acquire (int kind, bool audioStopped);
    void release();
    bool ensureCapacity (int samples);
    void freeBlocksFrom (int firstBlock);

    //==============================================================================
    double sampleRate = 0.0;
    int maxSamples = 0, maxBlocks = 0;

    // Loop memory: fixed-size blocks, published to the audio thread through capacityBlocks.
    std::vector<std::unique_ptr<float[]>> storage[4]; // loop L, loop R, undo L, undo R
    std::vector<float*> loopPtr[2], undoPtr[2];
    std::atomic<int> capacityBlocks { 0 };
    std::vector<std::uint8_t> chunkState;

    // rate-dependent constants
    int fadeSamples = 384, preRollSamples = 240, minLength = 4800, holdSamples = 38400, tapSamples = 19200;
    float smoothing = 0.002f;

    // --- audio-thread state
    State state = State::Empty;
    Settings cur;
    int length = 0, recPos = 0, targetLength = 0, capacitySamples = 0;
    double pos = 0.0, step = 1.0, lastAdvance = 0.0;
    double loopBpm = 0.0, recordBpm = 0.0;
    Scheduled scheduled = Scheduled::None;
    bool armThreshold = false;

    float playGain = 0.0f, playTarget = 0.0f, playStep = 0.0f;
    bool stopping = false, stopAtEnd = false, clearPending = false;
    float dubGain = 0.0f, dubTarget = 0.0f, dubStep = 0.0f;
    float prevIn[2] { 0.0f, 0.0f };
    bool firstDubSample = false;

    std::vector<PendingWrite> pending;
    int pendingCount = 0;
    bool pendingActive = false;
    std::int64_t pendingStartAt = 0;

    bool sessionActive = false, undone = false;
    int layers = 0, usedChunks = 0, dirtyChunks = 0, restorePending = 0, sweepCursor = 0;

    std::vector<float> ring[2], preStart[2];
    int ringPos = 0;

    std::int64_t clock = 0, mainDownAt = 0, lastMainDownAt = -1000000, stopDownAt = 0;
    bool mainHeld = false, mainHoldFired = false, stopHeld = false, stopHoldFired = false, lastPressEligible = false;
    PressAction pressAction = PressAction::None;

    double bpm = 120.0, beatsPerBar = 4.0, grid = 0.0, beatInc = 0.0;
    double lastBeatIndex = 0.0, lastBarIndex = 0.0;
    bool hostWasPlaying = false, hostPlaying = false;
    float clickEnv = 0.0f, clickDecay = 0.999f;
    double clickPhase = 0.0, clickInc = 0.0;

    float levelSm = 1.0f, monitorSm = 1.0f;
    int heldKind = 0;

    // --- shared with other threads
    std::mutex accessMutex;
    std::atomic<int> holdKind { 0 };            // 0 none, 1 read, 2 exclusive
    std::atomic<std::uint32_t> requestSerial { 0 }, grantedSerial { 0 }, deniedSerial { 0 }, blockCounter { 0 }, generation { 0 },
                               loopId { 0 };
    std::atomic<bool> abortRequested { false }, commandPending { false };
    std::atomic<bool> audioGuard { false };         // taken by whichever thread is working on the audio-thread state
    std::atomic<std::int64_t> lastBlockTime { 0 };  // when the last audio block started
    Command command;
    bool offlineHold = false;

    struct SharedStatus
    {
        std::atomic<int> state { 0 }, layers { 0 }, length { 0 }, recPos { 0 }, target { 0 };
        std::atomic<bool> scheduled { false }, stopping { false }, canUndo { false }, canRedo { false };
        std::atomic<double> position { 0.0 }, bpm { 120.0 }, loopBpm { 0.0 }, beatInBar { 0.0 }, beatsPerBar { 4.0 }, rate { 48000.0 };
    } shared;

    LooperEngine (const LooperEngine&) = delete;
    LooperEngine& operator= (const LooperEngine&) = delete;
};

} // namespace oap::looper
