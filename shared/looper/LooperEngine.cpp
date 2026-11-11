#include "LooperEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

namespace oap::looper
{

namespace
{
    // Loop memory is handed to the audio thread in blocks; undo works on smaller chunks.
    constexpr int blockShift = 16, blockSize = 1 << blockShift, blockMask = blockSize - 1;
    constexpr int chunkShift = 12, chunkSize = 1 << chunkShift, chunkMask = chunkSize - 1;
    constexpr int ringSize = 8192, ringMask = ringSize - 1;

    // What a chunk's two buffers hold:
    constexpr std::uint8_t chunkClean = 0;    // `loop` is what you hear
    constexpr std::uint8_t chunkSnapshot = 1; // `undo` holds the chunk as it was before the current overdub layer
    constexpr std::uint8_t chunkRestore = 2;  // `undo` is what you hear; `loop` still has to be restored from it

    constexpr double headroomSeconds = 10.0;
    constexpr float autoStartThreshold = 0.01f; // -40 dBFS
    constexpr int commandAdopt = 1, commandResize = 2;
    constexpr int holdRead = 1, holdExclusive = 2;
    constexpr int holdOffline = 3; // audio thread only: another thread works on the looper as if audio weren't running

    // When no audio block has started for this long, audio isn't running (the longest
    // block a host could reasonably ask for is far shorter).
    constexpr std::int64_t quietNanoseconds = 1000000000;

    int chunksFor (int samples) noexcept { return (samples + chunkSize - 1) >> chunkShift; }

    std::int64_t nanosecondsNow() noexcept
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

//==============================================================================
LooperEngine::LooperEngine()
{
    prepare (48000.0);
}

LooperEngine::~LooperEngine() = default;

void LooperEngine::prepare (double newRate)
{
    std::lock_guard<std::mutex> lock (accessMutex);
    lockOutAudio(); // (audio isn't running, so this never waits)
    newRate = std::max (8000.0, newRate);

    if (std::abs (newRate - sampleRate) < 1.0e-6)
    {
        // Same rate: keep the loop, drop anything that was half done.
        if (state == State::Recording || state == State::Armed)
            doClear();
        if (pendingActive)
            discardPending();
        if (state == State::Overdubbing)
            state = State::Playing;
        if (dubGain > 0.0f || dubTarget > 0.0f)
        {
            dubGain = dubTarget = 0.0f;
            bumpGeneration();
        }
        autoRun = false;
        autoPasses = autoPassesLeft = 0;
        rhythm.reset();
        mainHeld = stopHeld = false;
        scheduled = Scheduled::None;
        heldKind = 0;
        lastBlockTime.store (0);
        publishStatus();
        audioGuard.store (false);
        return;
    }

    capacityBlocks.store (0);
    freeBlocksFrom (0);

    sampleRate = newRate;
    maxSamples = (int) (maxSeconds * sampleRate);
    maxBlocks = maxSamples / blockSize + 2;

    for (auto& s : storage)
    {
        s.clear();
        s.resize ((size_t) maxBlocks);
    }
    for (auto* table : { &loopPtr[0], &loopPtr[1], &undoPtr[0], &undoPtr[1] })
        table->assign ((size_t) maxBlocks, nullptr);
    chunkState.assign ((size_t) maxBlocks * (size_t) (blockSize / chunkSize), chunkClean);

    fadeSamples = std::clamp ((int) (0.008 * sampleRate), 16, 3000);
    preRollSamples = std::clamp ((int) (0.005 * sampleRate), 8, 2000);
    minLength = std::max (4 * fadeSamples, (int) (0.1 * sampleRate));
    holdSamples = (int) (0.8 * sampleRate);
    tapSamples = (int) (0.4 * sampleRate);
    smoothing = 1.0f - (float) std::exp (-1.0 / (0.01 * sampleRate));
    rhythm.prepare (sampleRate);

    for (auto& r : ring)
        r.assign ((size_t) ringSize, 0.0f);
    for (auto& p : preStart)
        p.assign ((size_t) fadeSamples, 0.0f);
    pending.assign ((size_t) (2.0 * sampleRate) + 64, PendingWrite {});

    dirtyChunks = 0;
    length = 1; // makes doClear() count as a change
    doClear();
    clock = 0;
    lastMainDownAt = -1000000;
    mainHeld = stopHeld = false;
    grid = 0.0;
    lastBeatIndex = lastBarIndex = -1.0;
    hostWasPlaying = false;
    heldKind = 0;
    lastBlockTime.store (0);
    levelSm = monitorSm = 1.0f;

    ensureCapacity ((int) (headroomSeconds * sampleRate));
    publishStatus();
    audioGuard.store (false);
}

//==============================================================================
// Memory (other threads)

bool LooperEngine::ensureCapacity (int samples)
{
    samples = std::min (samples, maxSamples);
    const int needed = std::min (maxBlocks, samples / blockSize + 1);
    int have = capacityBlocks.load();

    try
    {
        while (have < needed)
        {
            for (int s = 0; s < 4; ++s)
                storage[s][(size_t) have] = std::unique_ptr<float[]> (new float[(size_t) blockSize]()); // zeroed, so the pages are touched here

            loopPtr[0][(size_t) have] = storage[0][(size_t) have].get();
            loopPtr[1][(size_t) have] = storage[1][(size_t) have].get();
            undoPtr[0][(size_t) have] = storage[2][(size_t) have].get();
            undoPtr[1][(size_t) have] = storage[3][(size_t) have].get();
            capacityBlocks.store (++have); // published only once the block is complete
        }
    }
    catch (const std::bad_alloc&)
    {
        for (int s = 0; s < 4; ++s)
            storage[s][(size_t) have].reset();
    }

    return have * blockSize >= samples;
}

void LooperEngine::freeBlocksFrom (int firstBlock)
{
    for (int b = std::max (0, firstBlock); b < (int) loopPtr[0].size(); ++b)
    {
        loopPtr[0][(size_t) b] = loopPtr[1][(size_t) b] = undoPtr[0][(size_t) b] = undoPtr[1][(size_t) b] = nullptr;
        for (auto& s : storage)
            s[(size_t) b].reset();
    }
}

void LooperEngine::serviceMemory()
{
    std::unique_lock<std::mutex> lock (accessMutex, std::try_to_lock);
    if (! lock.owns_lock() || sampleRate <= 0.0)
        return;

    const int headroom = (int) (headroomSeconds * sampleRate);
    const auto st = (State) shared.state.load();
    const int used = st == State::Recording ? shared.recPos.load() : shared.length.load();
    ensureCapacity (std::max (used, shared.target.load()) + headroom);

    // Give memory back after a clear. Safe without stopping audio: an empty looper touches no
    // loop memory, and a new recording starts at the beginning, far below what is kept.
    if (st == State::Empty && ! commandPending.load() && holdKind.load() == 0)
    {
        const int keep = std::min (maxBlocks, headroom / blockSize + 2);
        if (capacityBlocks.load() > keep)
        {
            capacityBlocks.store (keep);
            freeBlocksFrom (keep);
        }
    }
}

//==============================================================================
// Handshake with other threads

bool LooperEngine::isWriting() const noexcept
{
    return state == State::Recording || state == State::Armed || state == State::Overdubbing
        || dubGain > 0.0f || dubTarget > 0.0f || pendingActive || clearPending;
}

void LooperEngine::serviceHold() noexcept
{
    blockCounter.fetch_add (1);
    const int kind = holdKind.load();

    if (kind == 0)
    {
        heldKind = 0;
        if (commandPending.load())
        {
            executeCommand();
            commandPending.store (false);
        }
        return;
    }

    const auto serial = requestSerial.load();
    if (heldKind != 0 && grantedSerial.load() == serial)
        return; // still held by the same request

    heldKind = 0;
    if (commandPending.load()) // left by the previous request
    {
        executeCommand();
        commandPending.store (false);
    }

    if (isWriting())
    {
        deniedSerial.store (serial);
        return;
    }

    if (kind == holdExclusive)
    {
        if (state == State::Playing)
            finishStop();
        scheduled = Scheduled::None;
    }

    heldKind = kind;
    publishStatus();
    grantedSerial.store (serial);
}

bool LooperEngine::lockOutAudio()
{
    // The audio thread is only ever inside the looper for a moment.
    for (int i = 0; i < 20000; ++i)
    {
        if (! audioGuard.exchange (true))
            return true;
        std::this_thread::sleep_for (std::chrono::microseconds (100));
    }

    return false;
}

bool LooperEngine::acquire (int kind, bool audioStopped)
{
    abortRequested.store (false);
    offlineHold = false;
    const auto serial = requestSerial.fetch_add (1) + 1;
    holdKind.store (kind);

    // While audio runs, the audio thread answers the request with its next block. When no block
    // has started for a long time, audio isn't running: this thread then locks the audio thread
    // out and does what it would have done. Audio may start again at any moment; a block that
    // finds itself locked out leaves the looper alone.
    bool granted = false, offline = false;

    while (! granted && ! offline)
    {
        const auto block = blockCounter.load();

        if (audioStopped)
        {
            offline = lockOutAudio();
            if (! offline)
                break;
        }
        else if (grantedSerial.load() == serial)
        {
            granted = true;
        }
        else if (deniedSerial.load() == serial)
        {
            break;
        }
        else if (nanosecondsNow() - lastBlockTime.load() > quietNanoseconds)
        {
            if (! lockOutAudio())
                break;

            if (blockCounter.load() == block)
                offline = true;
            else
                audioGuard.store (false); // a block has started just now after all: wait for its answer
        }
        else
        {
            std::this_thread::sleep_for (std::chrono::microseconds (200));
        }
    }

    if (! granted && ! offline)
    {
        holdKind.store (0);
        return false;
    }

    if (offline)
    {
        offlineHold = true;

        if (commandPending.load())
        {
            executeCommand();
            commandPending.store (false);
        }

        if (pendingActive)
            discardPending();
        if (state == State::Recording || state == State::Armed || clearPending)
            doClear();
        if (state == State::Overdubbing)
            state = State::Playing;
        dubGain = dubTarget = 0.0f;
        autoRun = false;

        if (kind == holdExclusive)
        {
            if (state == State::Playing)
                finishStop();
            scheduled = Scheduled::None;
        }
        publishStatus();
    }

    return true;
}

void LooperEngine::release()
{
    if (offlineHold)
    {
        offlineHold = false;
        if (commandPending.load())
        {
            executeCommand();
            commandPending.store (false);
        }

        holdKind.store (0);
        audioGuard.store (false);
        return;
    }

    holdKind.store (0);
}

void LooperEngine::executeCommand() noexcept
{
    lastPressEligible = false;

    switch (command.type)
    {
        case commandAdopt:
        {
            doClear();
            loopId.fetch_add (1);
            length = command.length;
            if (length > 0)
            {
                state = State::Stopped;
                layers = 1;
                usedChunks = dirtyChunks = chunksFor (length);
                loopBpm = command.bpm;
            }
            break;
        }

        case commandResize:
        {
            const int newLength = command.length;
            if (length <= 0 || newLength <= 0 || state == State::Recording || state == State::Armed)
                break;

            commitLayer();

            if (newLength < length)
            {
                // The old end of the loop led seamlessly into its start, so fading the new end
                // into it makes the new loop point seamless too.
                const int n = std::min (fadeSamples, newLength / 2);
                for (int j = 0; j < n; ++j)
                {
                    const int src = length - n + j, dst = newLength - n + j;
                    const float f = (float) (j + 1) / (float) (n + 1);
                    const float sl = readAudible (0, src), sr = readAudible (1, src);
                    resolveChunk (dst >> chunkShift);
                    auto& l = loopPtr[0][(size_t) (dst >> blockShift)][dst & blockMask];
                    auto& r = loopPtr[1][(size_t) (dst >> blockShift)][dst & blockMask];
                    l += f * (sl - l);
                    r += f * (sr - r);
                }
            }

            length = newLength;
            usedChunks = chunksFor (length);
            dirtyChunks = std::max (dirtyChunks, usedChunks);
            pos = std::fmod (pos, (double) length);
            if (pos < 0.0)
                pos = 0.0;
            lastAdvance = 0.0;
            break;
        }

        default:
            break;
    }

    command.type = 0;
    publishStatus();
}

//==============================================================================
// Other threads: copy, load, resize

LooperEngine::Result LooperEngine::copyLoop (std::vector<float>& left, std::vector<float>& right, double& bpmOfLoop,
                                             std::uint32_t& generationOfCopy, bool audioStopped)
{
    std::lock_guard<std::mutex> lock (accessMutex);

    const auto expected = (size_t) std::max (0, shared.length.load());
    left.reserve (expected);
    right.reserve (expected);

    if (! acquire (holdRead, audioStopped))
        return Result::busy;

    generationOfCopy = generation.load();
    bpmOfLoop = loopBpm;
    const int len = (state == State::Empty) ? 0 : length;

    if (len <= 0)
    {
        left.clear();
        right.clear();
        release();
        return Result::empty;
    }

    left.resize ((size_t) len);
    right.resize ((size_t) len);

    for (int start = 0; start < len; start += chunkSize)
    {
        if (abortRequested.load())
        {
            release();
            return Result::busy;
        }

        const auto st = chunkState[(size_t) (start >> chunkShift)];
        const bool useUndo = st == chunkRestore || (st == chunkSnapshot && undone);
        const auto& source = useUndo ? undoPtr : loopPtr;
        const auto n = (size_t) std::min (chunkSize, len - start);
        const auto block = (size_t) (start >> blockShift);
        const int offset = start & blockMask;

        std::memcpy (left.data() + start, source[0][block] + offset, n * sizeof (float));
        std::memcpy (right.data() + start, source[1][block] + offset, n * sizeof (float));
    }

    release();
    return Result::ok;
}

LooperEngine::Result LooperEngine::loadLoop (const float* left, const float* right, int numSamples, double bpmOfLoop,
                                             bool audioStopped)
{
    std::lock_guard<std::mutex> lock (accessMutex);

    auto result = Result::ok;
    numSamples = (left == nullptr) ? 0 : std::max (0, numSamples);
    if (right == nullptr)
        right = left;

    if (numSamples > maxSamples)
    {
        numSamples = maxSamples;
        result = Result::tooLong;
    }

    if (numSamples > 0 && ! ensureCapacity (numSamples + (int) (headroomSeconds * sampleRate))
        && capacityBlocks.load() * blockSize < numSamples)
        return Result::tooLong; // out of memory

    if (! acquire (holdExclusive, audioStopped))
        return Result::busy;

    for (int start = 0; start < numSamples; start += blockSize)
    {
        const auto n = (size_t) std::min (blockSize, numSamples - start);
        const auto block = (size_t) (start >> blockShift);
        std::memcpy (loopPtr[0][block], left + start, n * sizeof (float));
        std::memcpy (loopPtr[1][block], right + start, n * sizeof (float));
    }

    command.type = commandAdopt;
    command.length = numSamples;
    command.bpm = bpmOfLoop;
    bumpGeneration();
    commandPending.store (true);
    release();
    return result;
}

LooperEngine::Result LooperEngine::doubleLength (bool audioStopped)
{
    std::lock_guard<std::mutex> lock (accessMutex);

    const auto st = (State) shared.state.load();
    const int expected = shared.length.load();
    if (st == State::Empty || expected <= 0)
        return Result::empty;
    if (st != State::Playing && st != State::Stopped)
        return Result::busy;
    if (expected > maxSamples / 2)
        return Result::tooLong;
    if (! ensureCapacity (2 * expected + (int) (headroomSeconds * sampleRate))
        && capacityBlocks.load() * blockSize < 2 * expected)
        return Result::tooLong;

    if (! acquire (holdRead, audioStopped))
        return Result::busy;

    const int len = length;
    if (len <= 0 || len != expected || state == State::Empty)
    {
        release();
        return Result::busy;
    }

    // The second half is written to both buffers, so it is right whichever one a chunk is heard from.
    for (int done = 0; done < len;)
    {
        if (abortRequested.load())
        {
            release();
            return Result::busy;
        }

        const int dst = len + done;
        const int n = std::min ({ chunkSize - (done & chunkMask), blockSize - (dst & blockMask), len - done });
        const auto st2 = chunkState[(size_t) (done >> chunkShift)];
        const bool useUndo = st2 == chunkRestore || (st2 == chunkSnapshot && undone);
        const auto& source = useUndo ? undoPtr : loopPtr;

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* src = source[ch][(size_t) (done >> blockShift)] + (done & blockMask);
            std::memcpy (loopPtr[ch][(size_t) (dst >> blockShift)] + (dst & blockMask), src, (size_t) n * sizeof (float));
            std::memcpy (undoPtr[ch][(size_t) (dst >> blockShift)] + (dst & blockMask), src, (size_t) n * sizeof (float));
        }
        done += n;
    }

    command.type = commandResize;
    command.length = 2 * len;
    bumpGeneration();
    commandPending.store (true);
    release();
    return Result::ok;
}

LooperEngine::Result LooperEngine::halveLength (bool audioStopped)
{
    std::lock_guard<std::mutex> lock (accessMutex);

    const auto st = (State) shared.state.load();
    if (st == State::Empty || shared.length.load() <= 0)
        return Result::empty;
    if (st != State::Playing && st != State::Stopped)
        return Result::busy;

    if (! acquire (holdRead, audioStopped))
        return Result::busy;

    const int newLength = length / 2;
    if (state == State::Empty || newLength < minLength)
    {
        release();
        return Result::tooLong; // too short to halve
    }

    command.type = commandResize;
    command.length = newLength;
    bumpGeneration();
    commandPending.store (true);
    release();
    return Result::ok;
}

//==============================================================================
// Loop memory (audio thread)

inline float LooperEngine::readAudible (int channel, int index) const noexcept
{
    const auto st = chunkState[(size_t) (index >> chunkShift)];
    const bool useUndo = st == chunkRestore || (st == chunkSnapshot && undone);
    return (useUndo ? undoPtr : loopPtr)[channel][(size_t) (index >> blockShift)][index & blockMask];
}

void LooperEngine::copyChunk (int chunk, bool undoToLoop) noexcept
{
    const int base = chunk << chunkShift;
    const auto block = (size_t) (base >> blockShift);
    const int offset = base & blockMask;

    for (int ch = 0; ch < 2; ++ch)
    {
        float* l = loopPtr[ch][block] + offset;
        float* u = undoPtr[ch][block] + offset;
        std::memcpy (undoToLoop ? l : u, undoToLoop ? u : l, (size_t) chunkSize * sizeof (float));
    }
}

void LooperEngine::resolveChunk (int chunk) noexcept
{
    auto& st = chunkState[(size_t) chunk];
    if (st == chunkRestore)
    {
        copyChunk (chunk, true);
        st = chunkClean;
        --restorePending;
    }
}

void LooperEngine::prepareChunkForWrite (int chunk) noexcept
{
    auto& st = chunkState[(size_t) chunk];
    if (st == chunkSnapshot)
        return;

    if (st == chunkRestore)
    {
        copyChunk (chunk, true); // `undo` already is the chunk before this layer
        --restorePending;
    }
    else
    {
        copyChunk (chunk, false); // keep the chunk as it is now, for undo
    }
    st = chunkSnapshot;
}

void LooperEngine::writeOverdub (int index, float l, float r, float feedback) noexcept
{
    prepareChunkForWrite (index >> chunkShift);
    const auto block = (size_t) (index >> blockShift);
    const int offset = index & blockMask;
    loopPtr[0][block][offset] = loopPtr[0][block][offset] * feedback + l;
    loopPtr[1][block][offset] = loopPtr[1][block][offset] * feedback + r;
}

/** Makes the current overdub layer permanent (or drops it, if it was undone). */
void LooperEngine::commitLayer() noexcept
{
    if (sessionActive)
    {
        for (int c = 0; c < dirtyChunks; ++c)
        {
            auto& st = chunkState[(size_t) c];
            if (st == chunkSnapshot)
            {
                if (undone)
                {
                    st = chunkRestore;
                    ++restorePending;
                }
                else
                {
                    st = chunkClean;
                }
            }
        }
    }

    sessionActive = false;
    undone = false;
}

void LooperEngine::startSession() noexcept
{
    commitLayer();
    sessionActive = true;
    ++layers;
}

void LooperEngine::commitPending() noexcept
{
    pendingActive = false;
    startSession();
    for (int i = 0; i < pendingCount; ++i)
    {
        const auto& p = pending[(size_t) i];
        writeOverdub (p.index, p.left, p.right, p.feedback);
    }
    pendingCount = 0;
}

void LooperEngine::discardPending() noexcept
{
    pendingActive = false;
    pendingCount = 0;
    dubGain = dubTarget = 0.0f;
    if (state == State::Overdubbing)
        state = State::Playing;
}

/** Restores a few chunks of an undone layer per block, so the work is spread out. */
void LooperEngine::sweep() noexcept
{
    int budget = 8, scans = 512;
    while (restorePending > 0 && budget > 0 && scans-- > 0 && dirtyChunks > 0)
    {
        if (sweepCursor >= dirtyChunks)
            sweepCursor = 0;
        if (chunkState[(size_t) sweepCursor] == chunkRestore)
        {
            resolveChunk (sweepCursor);
            --budget;
        }
        ++sweepCursor;
    }
}

//==============================================================================
// Transport (audio thread)

bool LooperEngine::quantizeActive() const noexcept
{
    // Without a playing host or the click there is no grid to hear, so waiting for it would only feel like lag.
    return cur.sync && cur.quantize != Quantize::Off && (hostPlaying || cur.click);
}

void LooperEngine::startRecording (bool withPreRoll) noexcept
{
    const int pre = (withPreRoll && capacitySamples > preRollSamples) ? preRollSamples : 0;

    // Remember what was played just before the loop starts, for the loop-point crossfade.
    for (int ch = 0; ch < 2; ++ch)
        for (int j = 0; j < fadeSamples; ++j)
            preStart[ch][(size_t) j] = ring[ch][(size_t) ((ringPos - pre - fadeSamples + j) & ringMask)];

    // Auto start: keep the few milliseconds before the trigger so the first attack is complete.
    for (int ch = 0; ch < 2; ++ch)
        for (int j = 0; j < pre; ++j)
            loopPtr[ch][0][j] = ring[ch][(size_t) ((ringPos - pre + j) & ringMask)];

    recPos = pre;
    targetLength = 0;
    recordBpm = (cur.sync || autoRun) ? bpm : 0.0;

    if (cur.sync || autoRun)
    {
        double beats = cur.fixedBars > 0.0 ? cur.fixedBars * beatsPerBar : cur.fixedBeats;
        if (autoRun && beats <= 0.0)
            beats = 4.0 * beatsPerBar; // an auto loop always has a length: four bars unless one is chosen

        if (beats > 0.0)
        {
            double samples = beats * 60.0 / bpm * sampleRate;
            if (autoRun)
                samples = std::min (samples, (double) maxSamples);
            if (samples >= (double) minLength && samples <= (double) maxSamples)
                targetLength = (int) std::llround (samples);
        }
    }

    state = State::Recording;
    scheduled = Scheduled::None;
    armThreshold = false;
}

void LooperEngine::startAutoLoop() noexcept
{
    autoRun = true;
    autoPasses = autoPassesLeft = 0;
    state = State::Armed;
    armThreshold = false;
    scheduled = Scheduled::StartRecord;

    // When nothing keeps time yet (no host transport, no rhythm playing), the first bar starts now.
    placeAutoStart (! hostPlaying && ! cur.click);
}

void LooperEngine::placeAutoStart (bool restartClock) noexcept
{
    const int bars = std::clamp (cur.countInBars, 0, 8);

    if (restartClock)
    {
        grid = 0.0;
        lastBeatIndex = lastBarIndex = -1.0;
        recordAt = bars * beatsPerBar;
    }
    else
    {
        // The first bar line at least `bars` bars away (with half a beat of slack, so that a press
        // just after a bar line doesn't wait a whole extra bar).
        const double earliest = grid + bars * beatsPerBar - (bars > 0 ? 0.5 : 0.0);
        recordAt = beatsPerBar * std::ceil (earliest / beatsPerBar - 1.0e-9);
    }

    countInLength = std::max (0.0, recordAt - grid);
}

void LooperEngine::closeLoop() noexcept
{
    length = recPos;
    scheduled = Scheduled::None;

    if (length < minLength)
    {
        doClear();
        return;
    }

    // Crossfade the end of the loop into what was played just before it began: the loop point
    // becomes seamless and the attack at the start of the loop stays untouched.
    const int n = std::min (fadeSamples, length / 2);
    for (int j = 0; j < n; ++j)
    {
        const int index = length - n + j;
        const float f = (float) (j + 1) / (float) (n + 1);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto& s = loopPtr[ch][(size_t) (index >> blockShift)][index & blockMask];
            s += f * (preStart[ch][(size_t) (fadeSamples - n + j)] - s);
        }
    }

    usedChunks = dirtyChunks = chunksFor (length);
    state = State::Playing;
    pos = cur.reverse ? (double) (length - 1) : 0.0;
    lastAdvance = 0.0;
    playGain = playTarget = 1.0f;
    stopping = stopAtEnd = false;
    layers = 1;
    sessionActive = undone = false;
    targetLength = 0;
    loopBpm = recordBpm;
    bumpGeneration();
    loopId.fetch_add (1);

    if (autoRun)
    {
        // Auto loop: the layers follow straight away, one loop cycle each.
        autoPasses = autoPassesLeft = std::clamp (cur.autoLayers, 0, 8);
        if (autoPassesLeft > 0)
            startOverdub();
        else
            autoRun = false;
    }
    else if (cur.overdubAfterRecord)
    {
        startOverdub();
    }
}

void LooperEngine::startPlayback() noexcept
{
    if (length <= 0)
        return;

    pos = cur.reverse ? (double) (length - 1) : 0.0;
    lastAdvance = 0.0;
    state = State::Playing;
    playGain = 0.0f;
    playTarget = 1.0f;
    playStep = (float) (1.0 / (0.002 * sampleRate));
    stopping = stopAtEnd = false;
    scheduled = Scheduled::None;
}

void LooperEngine::startOverdub() noexcept
{
    if (state != State::Playing)
        return;

    // The new layer is collected on the side until it is clear that this press wasn't the start of
    // a double tap or a hold; only then is it written into the loop (and the previous layer made permanent).
    state = State::Overdubbing;
    pendingActive = true;
    pendingCount = 0;
    pendingStartAt = clock;
    dubTarget = 1.0f;
    dubStep = (float) (1.0 / (0.005 * sampleRate));
    firstDubSample = true;
}

void LooperEngine::endOverdub() noexcept
{
    if (pendingActive)
        commitPending();
    if (state == State::Overdubbing)
        state = State::Playing;
    dubTarget = 0.0f;
}

void LooperEngine::beginStop() noexcept
{
    autoRun = false;
    if (state != State::Playing)
        return;

    switch (cur.stopMode)
    {
        case StopMode::FadeOut:
            stopping = true;
            playTarget = 0.0f;
            playStep = (float) (1.0 / (3.0 * sampleRate));
            break;

        case StopMode::LoopEnd:
            stopAtEnd = true;
            break;

        case StopMode::Instant:
        default:
            stopping = true;
            playTarget = 0.0f;
            playStep = (float) (1.0 / (0.01 * sampleRate));
            break;
    }
}

void LooperEngine::cancelStop() noexcept
{
    stopping = stopAtEnd = false;
    playTarget = 1.0f;
    playStep = (float) (1.0 / (0.02 * sampleRate));
}

void LooperEngine::finishStop() noexcept
{
    autoRun = false;
    if (pendingActive)
        discardPending();
    if (dubGain > 0.0f || dubTarget > 0.0f)
    {
        dubGain = dubTarget = 0.0f;
        bumpGeneration();
    }

    state = State::Stopped;
    stopping = stopAtEnd = false;
    playGain = playTarget = 0.0f;
    pos = 0.0;
    lastAdvance = 0.0;
}

void LooperEngine::requestClear() noexcept
{
    lastPressEligible = false;
    autoRun = false;
    if (state == State::Empty)
        return;

    if (state == State::Playing || state == State::Overdubbing)
    {
        if (pendingActive)
            discardPending();
        dubGain = dubTarget = 0.0f;
        state = State::Playing;
        clearPending = true; // after a very short fade, so clearing doesn't click
        playTarget = 0.0f;
        playStep = (float) (1.0 / (0.005 * sampleRate));
    }
    else
    {
        doClear();
    }
}

void LooperEngine::doClear() noexcept
{
    const bool hadSomething = length > 0 || state != State::Empty;

    std::fill (chunkState.begin(), chunkState.begin() + dirtyChunks, chunkClean);
    length = recPos = targetLength = 0;
    pos = lastAdvance = 0.0;
    state = State::Empty;
    scheduled = Scheduled::None;
    armThreshold = false;
    playGain = playTarget = 0.0f;
    stopping = stopAtEnd = clearPending = false;
    dubGain = dubTarget = 0.0f;
    pendingActive = false;
    pendingCount = 0;
    sessionActive = undone = false;
    layers = usedChunks = dirtyChunks = restorePending = sweepCursor = 0;
    loopBpm = 0.0;
    autoRun = false;
    autoPasses = autoPassesLeft = 0;
    countInLength = 0.0;

    if (hadSomething)
    {
        bumpGeneration();
        loopId.fetch_add (1);
    }
}

void LooperEngine::clear() noexcept
{
    if (! enterAudio())
        return;

    requestClear();
    publishStatus();
    leaveAudio();
}

void LooperEngine::toggleUndo() noexcept
{
    if (! sessionActive || length <= 0)
        return;

    undone = ! undone;
    layers += undone ? -1 : 1;
    bumpGeneration();
}

void LooperEngine::undoSwitch() noexcept
{
    if (! enterAudio())
        return;

    undoAction();
    publishStatus();
    leaveAudio();
}

void LooperEngine::undoAction() noexcept
{
    lastPressEligible = false;
    autoRun = false;
    if (state == State::Armed || state == State::Recording)
    {
        doClear(); // cancel the recording
        return;
    }

    if (pendingActive)
    {
        discardPending(); // cancel the overdub that has just started
        return;
    }

    if (state == State::Overdubbing || dubGain > 0.0f || dubTarget > 0.0f)
    {
        // Cancel the overdub in progress: it becomes the layer that redo can bring back.
        dubGain = dubTarget = 0.0f;
        if (state == State::Overdubbing)
            state = State::Playing;
        if (sessionActive && ! undone)
        {
            undone = true;
            --layers;
        }
        bumpGeneration();
        return;
    }

    toggleUndo();
}

//==============================================================================
// Switches (audio thread)

void LooperEngine::mainSwitch (bool down) noexcept
{
    if (! enterAudio())
        return;

    if (down)
    {
        mainPressed();
        publishStatus();
    }
    else
    {
        mainHeld = false;
    }

    leaveAudio();
}

void LooperEngine::mainPressed() noexcept
{

    mainHeld = true;
    mainHoldFired = false;
    mainDownAt = clock;

    const bool doubleTap = lastPressEligible && (clock - lastMainDownAt) <= tapSamples;
    lastMainDownAt = clock;

    // Pressing during an auto loop takes over: the press does what it always does, and nothing
    // more happens by itself.
    autoRun = false;

    if (doubleTap)
    {
        // Second press of a double tap: whatever the first press started is taken back, then stop.
        lastPressEligible = false;
        pressAction = PressAction::None;
        if (pendingActive)
            discardPending();
        if (state == State::Overdubbing)
            endOverdub();
        if (state == State::Playing)
            beginStop();
        return;
    }

    pressAction = PressAction::None;

    switch (state)
    {
        case State::Empty:
            if (cur.autoLoop)
            {
                startAutoLoop();
            }
            else if (cur.autoStart)
            {
                state = State::Armed;
                armThreshold = true;
            }
            else if (quantizeActive())
            {
                state = State::Armed;
                armThreshold = false;
                scheduled = Scheduled::StartRecord;
            }
            else
            {
                startRecording (false);
            }
            pressAction = PressAction::StartedRecord;
            break;

        case State::Armed:
            state = State::Empty;
            scheduled = Scheduled::None;
            armThreshold = false;
            break;

        case State::Recording:
            if (scheduled != Scheduled::CloseRecord && quantizeActive())
            {
                scheduled = Scheduled::CloseRecord;
            }
            else
            {
                closeLoop();
                if (state == State::Playing || state == State::Overdubbing)
                    pressAction = PressAction::ClosedRecord;
            }
            break;

        case State::Playing:
            if (stopping || stopAtEnd || clearPending)
            {
                if (! clearPending)
                    cancelStop();
            }
            else
            {
                startOverdub();
                pressAction = PressAction::StartedOverdub;
            }
            break;

        case State::Overdubbing:
            endOverdub();
            pressAction = PressAction::EndedOverdub;
            break;

        case State::Stopped:
            if (scheduled == Scheduled::StartPlay)
            {
                scheduled = Scheduled::None;
            }
            else if (quantizeActive())
            {
                scheduled = Scheduled::StartPlay;
            }
            else
            {
                startPlayback();
                pressAction = PressAction::StartedPlay;
            }
            break;

        default:
            break;
    }

    lastPressEligible = pressAction == PressAction::ClosedRecord || pressAction == PressAction::StartedOverdub
                     || pressAction == PressAction::EndedOverdub || pressAction == PressAction::StartedPlay;
}

void LooperEngine::onMainHold() noexcept
{
    lastPressEligible = false;

    switch (pressAction)
    {
        case PressAction::StartedOverdub:
            // The press started an overdub, but it turned out to be a hold: take the overdub back
            // and undo (or redo) the layer before it.
            if (pendingActive)
            {
                discardPending();
                toggleUndo();
            }
            else
            {
                undoAction();
            }
            break;

        case PressAction::EndedOverdub:
            undoAction(); // undo the layer that has just been recorded
            break;

        case PressAction::StartedPlay:
            requestClear(); // held while stopped
            break;

        case PressAction::None:
        case PressAction::StartedRecord:
        case PressAction::ClosedRecord:
        default:
            break;
    }

    pressAction = PressAction::None;
}

void LooperEngine::stopSwitch (bool down) noexcept
{
    if (! enterAudio())
        return;

    if (down)
    {
        stopPressed();
        publishStatus();
    }
    else
    {
        stopHeld = false;
    }

    leaveAudio();
}

void LooperEngine::stopPressed() noexcept
{
    autoRun = false;

    stopHeld = true;
    stopHoldFired = false;
    stopDownAt = clock;
    lastPressEligible = false;

    switch (state)
    {
        case State::Armed:
            state = State::Empty;
            scheduled = Scheduled::None;
            armThreshold = false;
            break;

        case State::Recording:
            closeLoop();
            if (state == State::Playing || state == State::Overdubbing)
                finishStop();
            break;

        case State::Playing:
        case State::Overdubbing:
            if (pendingActive)
                discardPending();
            if (state == State::Overdubbing)
                endOverdub();
            if (stopping || stopAtEnd)
                finishStop(); // second press: stop right now
            else
                beginStop();
            break;

        case State::Stopped:
            scheduled = Scheduled::None;
            break;

        case State::Empty:
        default:
            break;
    }
}

void LooperEngine::fireScheduled() noexcept
{
    const auto action = scheduled;

    switch (action)
    {
        case Scheduled::StartRecord:
            scheduled = Scheduled::None;
            if (state == State::Armed)
                startRecording (false);
            break;

        case Scheduled::CloseRecord:
            if (state != State::Recording)
                scheduled = Scheduled::None;
            else if (recPos >= minLength)
                closeLoop();
            break; // otherwise wait for the next line

        case Scheduled::StartPlay:
            scheduled = Scheduled::None;
            if (state == State::Stopped)
                startPlayback();
            break;

        case Scheduled::None:
        default:
            break;
    }
}

//==============================================================================
void LooperEngine::updateTiming (const Timing& t) noexcept
{
    bpm = std::isfinite (t.bpm) ? std::clamp (t.bpm, 20.0, 400.0) : 120.0;
    const double newBeatsPerBar = std::isfinite (t.beatsPerBar) ? std::clamp (t.beatsPerBar, 1.0, 32.0) : 4.0;
    beatInc = bpm / 60.0 / sampleRate;
    hostPlaying = t.hostPlaying;

    bool resync = std::abs (newBeatsPerBar - beatsPerBar) > 1.0e-9;
    beatsPerBar = newBeatsPerBar;

    // The time signature only decides how the rhythm groups the beats; when it doesn't match the
    // bar length, the bar length wins.
    int numerator = t.numerator, denominator = t.denominator;
    if (numerator < 1 || denominator < 1 || std::abs (4.0 * numerator / denominator - beatsPerBar) > 1.0e-6)
    {
        numerator = std::max (1, (int) std::lround (beatsPerBar));
        denominator = 4;
    }
    meter = { beatsPerBar, numerator, denominator };

    if (hostPlaying && std::isfinite (t.hostBeat) && std::isfinite (t.hostBarStart))
    {
        // A beat position whose multiples of beatsPerBar are the host's bar lines.
        const double hostGrid = (t.hostBeat - t.hostBarStart)
                              + beatsPerBar * std::floor (t.hostBarStart / beatsPerBar + 1.0e-9);
        if (! hostWasPlaying || std::abs (hostGrid - grid) > 0.25)
            resync = true; // transport started or jumped
        grid = hostGrid;
    }

    if (resync)
    {
        // A line exactly at the new position still counts.
        lastBeatIndex = std::floor (grid - 1.0e-9);
        lastBarIndex = std::floor ((grid - 1.0e-9) / beatsPerBar);

        // An auto loop counting in starts counting again from the new position.
        if (autoRun && state == State::Armed && scheduled == Scheduled::StartRecord)
            placeAutoStart (false);
    }

    hostWasPlaying = hostPlaying;

    double ratio = 1.0;
    if (cur.sync && loopBpm > 0.0)
    {
        ratio = std::clamp (bpm / loopBpm, 0.25, 4.0);
        if (std::abs (ratio - 1.0) < 1.0e-4)
            ratio = 1.0;
    }
    step = (cur.reverse ? -1.0 : 1.0) * (double) std::clamp (cur.speed, 0.25f, 4.0f) * ratio;
}

void LooperEngine::publishStatus() noexcept
{
    shared.state.store ((int) state);
    shared.layers.store (layers);
    shared.length.store (state == State::Recording ? recPos : length);
    shared.recPos.store (recPos);
    shared.target.store (targetLength);
    shared.scheduled.store (scheduled != Scheduled::None || state == State::Armed);
    shared.stopping.store (stopping || stopAtEnd);
    shared.canUndo.store (sessionActive && ! undone && length > 0);
    shared.canRedo.store (sessionActive && undone && length > 0);
    shared.position.store (pos);
    shared.bpm.store (bpm);
    shared.loopBpm.store (state == State::Recording ? recordBpm : loopBpm);
    shared.beatsPerBar.store (beatsPerBar);
    double inBar = std::fmod (grid, beatsPerBar);
    if (inBar < 0.0)
        inBar += beatsPerBar;
    shared.beatInBar.store (inBar);
    shared.rate.store (sampleRate);
    shared.numerator.store (meter.numerator);
    shared.denominator.store (meter.denominator);

    const bool countingIn = autoRun && state == State::Armed && scheduled == Scheduled::StartRecord;
    shared.autoRunning.store (autoRun);
    // (if the host's position steps back a little, the count-in takes a moment longer; it is shown as full)
    shared.countIn.store (countingIn ? std::clamp (recordAt - grid, 0.0, countInLength) : 0.0);
    shared.countInLength.store (countingIn ? countInLength : 0.0);
    shared.autoLayers.store (autoRun ? (autoPasses > 0 ? autoPasses : std::clamp (cur.autoLayers, 0, 8)) : 0);
    shared.autoLayer.store (autoRun && state == State::Overdubbing && autoPasses > 0 ? autoPasses - autoPassesLeft + 1 : 0);
}

Status LooperEngine::getStatus() const noexcept
{
    Status s;
    const double rate = std::max (1.0, shared.rate.load());
    s.state = (State) shared.state.load();
    s.scheduled = shared.scheduled.load();
    s.stopping = shared.stopping.load();
    s.canUndo = shared.canUndo.load();
    s.canRedo = shared.canRedo.load();
    s.layers = shared.layers.load();
    s.lengthSeconds = shared.length.load() / rate;
    s.positionSeconds = shared.position.load() / rate;
    s.targetSeconds = shared.target.load() / rate;
    s.bpm = shared.bpm.load();
    s.loopBpm = shared.loopBpm.load();
    s.beatInBar = shared.beatInBar.load();
    s.beatsPerBar = shared.beatsPerBar.load();
    s.numerator = shared.numerator.load();
    s.denominator = shared.denominator.load();
    s.generation = generation.load();
    s.autoRunning = shared.autoRunning.load();
    s.countIn = shared.countIn.load();
    s.countInLength = shared.countInLength.load();
    s.autoLayer = shared.autoLayer.load();
    s.autoLayers = shared.autoLayers.load();
    return s;
}

//==============================================================================
// The audio thread takes this (without ever waiting) around everything it does with the looper.
// It only fails while another thread works on the looper because audio seemed to have stopped
// (see acquire()); the block that finds that out passes the input through and does nothing else.
bool LooperEngine::enterAudio() noexcept
{
    if (heldKind == holdOffline)
        return false;

    if (audioGuard.exchange (true))
    {
        heldKind = holdOffline;
        return false;
    }

    return true;
}

void LooperEngine::leaveAudio() noexcept
{
    audioGuard.store (false);
}

void LooperEngine::beginBlock (const Settings& settings, const Timing& timing) noexcept
{
    lastBlockTime.store (nanosecondsNow());

    if (heldKind == holdOffline)
        heldKind = 0; // a new block: try again
    if (! enterAudio())
        return;

    serviceHold();
    cur = settings;
    capacitySamples = std::min (maxSamples, capacityBlocks.load() * blockSize);
    updateTiming (timing);
    leaveAudio();
}

void LooperEngine::process (float* left, float* right, int numSamples) noexcept
{
    if (! enterAudio())
    {
        // The looper is in another thread's hands for this block: only the input passes.
        if (! cur.monitor)
        {
            std::fill (left, left + numSamples, 0.0f);
            std::fill (right, right + numSamples, 0.0f);
        }
        return;
    }

    const bool exclusive = heldKind == holdExclusive;
    const int pattern = std::clamp (cur.rhythm, 0, RhythmGuide::numPatterns - 1);
    const float levelTarget = std::clamp (cur.level, 0.0f, 4.0f);
    const float monitorTarget = cur.monitor ? 1.0f : 0.0f;
    const float feedback = std::clamp (cur.feedback, 0.0f, 1.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = left[i], inR = right[i];

        // --- hold gestures and the double-tap window (not while another thread has the loop)
        if (heldKind == 0)
        {
            if (mainHeld && ! mainHoldFired && clock - mainDownAt >= holdSamples)
            {
                mainHoldFired = true;
                onMainHold();
            }

            if (stopHeld && ! stopHoldFired && clock - stopDownAt >= holdSamples)
            {
                stopHoldFired = true;
                requestClear();
            }

            if (pendingActive && ! mainHeld && clock - pendingStartAt >= tapSamples)
                commitPending();
        }

        // --- beat grid: quantised actions and the click
        bool beatTick = false, barTick = false;
        // (the tiny offset keeps a line that falls exactly on a sample from slipping to the next one)
        if (const double beatIndex = std::floor (grid + 1.0e-9); beatIndex > lastBeatIndex)
        {
            lastBeatIndex = beatIndex;
            beatTick = true;
        }
        if (const double barIndex = std::floor ((grid + 1.0e-9) / beatsPerBar); barIndex > lastBarIndex)
        {
            lastBarIndex = barIndex;
            barTick = true;
        }

        if (scheduled != Scheduled::None && ! exclusive)
        {
            if (autoRun && scheduled == Scheduled::StartRecord)
            {
                if (grid + 1.0e-9 >= recordAt)
                    fireScheduled(); // the count-in is over
            }
            else
            {
                const bool line = cur.quantize == Quantize::Beat ? beatTick : barTick;
                if (line || ! cur.sync || cur.quantize == Quantize::Off)
                    fireScheduled();
            }
        }

        // --- the guide rhythm: always when it is switched on, and through an auto loop. A count-in
        // is counted on the beats; the groove starts with the recording, like a drummer's count-in.
        const bool countingIn = autoRun && state == State::Armed;
        const bool groove = cur.click || (autoRun && ! countingIn);
        const float guide = rhythm.process (grid, meter, pattern, groove, countingIn && ! (cur.click && RhythmGuide::isClick (pattern)),
                                            cur.rhythmLevel);

        // --- auto start
        if (state == State::Armed && armThreshold && std::max (std::abs (inL), std::abs (inR)) > autoStartThreshold)
            startRecording (true);

        float loopL = 0.0f, loopR = 0.0f;

        if (state == State::Recording)
        {
            if (recPos < capacitySamples)
            {
                loopPtr[0][(size_t) (recPos >> blockShift)][recPos & blockMask] = inL;
                loopPtr[1][(size_t) (recPos >> blockShift)][recPos & blockMask] = inR;
                ++recPos;
            }

            if ((targetLength > 0 && recPos >= targetLength) || recPos >= capacitySamples)
                closeLoop(); // fixed length reached, or out of memory: the loop ends here
        }
        else if ((state == State::Playing || state == State::Overdubbing) && ! exclusive && length > 0)
        {
            // --- play head
            const int i0 = (int) pos;
            const float frac = (float) (pos - (double) i0);
            loopL = readAudible (0, i0);
            loopR = readAudible (1, i0);
            if (frac > 0.0f)
            {
                const int i1 = (i0 + 1 >= length) ? 0 : i0 + 1;
                loopL += frac * (readAudible (0, i1) - loopL);
                loopR += frac * (readAudible (1, i1) - loopR);
            }

            // --- record head: the input, resampled onto the loop positions passed since the last sample
            if (dubGain > 0.0f || dubTarget > 0.0f)
            {
                if (dubGain < dubTarget)
                {
                    dubGain = std::min (dubTarget, dubGain + dubStep);
                }
                else if (dubGain > dubTarget)
                {
                    dubGain = std::max (dubTarget, dubGain - dubStep);
                    if (dubGain <= 0.0f)
                        bumpGeneration(); // the layer is complete
                }

                if (firstDubSample)
                {
                    prevIn[0] = inL;
                    prevIn[1] = inR;
                    firstDubSample = false;
                }

                if (dubGain > 0.0f && (pendingActive || (sessionActive && ! undone)))
                {
                    const float fb = 1.0f - dubGain * (1.0f - feedback);
                    const double from = pos - lastAdvance;
                    const bool forward = lastAdvance > 0.0;
                    int k = forward ? (int) std::floor (from) + 1 : (int) std::ceil (from) - 1;
                    const int last = forward ? (int) std::floor (pos) : (int) std::ceil (pos);
                    const double span = std::abs (lastAdvance);

                    while (span > 0.0 && (forward ? k <= last : k >= last))
                    {
                        const float t = (float) (std::abs ((double) k - from) / span);
                        const float vl = (prevIn[0] + t * (inL - prevIn[0])) * dubGain;
                        const float vr = (prevIn[1] + t * (inR - prevIn[1])) * dubGain;
                        int index = k;
                        if (index < 0)
                            index += length;
                        else if (index >= length)
                            index -= length;

                        if (pendingActive && pendingCount >= (int) pending.size())
                            commitPending(); // side buffer full: write it out early

                        if (pendingActive)
                            pending[(size_t) pendingCount++] = { index, vl, vr, fb };
                        else
                            writeOverdub (index, vl, vr, fb);

                        k += forward ? 1 : -1;
                    }
                }
            }

            // --- "loop end" stop: fade just before the loop point
            if (stopAtEnd && playTarget > 0.0f)
            {
                const double remaining = step > 0.0 ? ((double) length - pos) / step : (pos + 1.0) / -step;
                if (remaining <= (double) fadeSamples)
                {
                    playTarget = 0.0f;
                    playStep = playGain / (float) std::max (1.0, remaining);
                }
            }

            // --- advance
            lastAdvance = step;
            pos += step;
            bool wrapped = false;
            if (pos >= (double) length)
            {
                pos -= (double) length;
                wrapped = true;
            }
            else if (pos < 0.0)
            {
                pos += (double) length;
                wrapped = true;
            }
            if (pos >= (double) length || pos < 0.0)
                pos = 0.0;

            if (wrapped && stopAtEnd)
                finishStop();

            // Auto loop: each automatic layer lasts one loop cycle.
            if (wrapped && autoRun && state == State::Overdubbing)
            {
                endOverdub();
                if (--autoPassesLeft > 0)
                {
                    startOverdub();
                }
                else
                {
                    autoRun = false;
                    autoPasses = 0;
                }
            }
        }

        // --- playback gain: fades for start, stop and clear
        const float gain = playGain;
        if (playGain < playTarget || playGain > playTarget)
        {
            playGain = playGain < playTarget ? std::min (playTarget, playGain + playStep)
                                             : std::max (playTarget, playGain - playStep);
            if (playGain <= 0.0f)
            {
                if (clearPending)
                    doClear();
                else if (stopping)
                    finishStop();
            }
        }

        levelSm += smoothing * (levelTarget - levelSm);
        monitorSm += smoothing * (monitorTarget - monitorSm);

        left[i] = inL * monitorSm + loopL * gain * levelSm + guide;
        right[i] = inR * monitorSm + loopR * gain * levelSm + guide;

        // --- history
        ring[0][(size_t) ringPos] = inL;
        ring[1][(size_t) ringPos] = inR;
        ringPos = (ringPos + 1) & ringMask;
        prevIn[0] = inL;
        prevIn[1] = inR;
        grid += beatInc;
        ++clock;
    }

    if (heldKind == 0 && restorePending > 0)
        sweep();

    publishStatus();
    leaveAudio();
}

} // namespace oap::looper
