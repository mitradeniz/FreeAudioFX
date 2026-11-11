#include "LooperProcessor.h"

#include "looper/LoopUtils.h"
#include "ui/LooperEditor.h"

#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>

namespace oap
{

using looper::LooperEngine;

namespace
{
    constexpr char stateMagic[8] = { 'O', 'A', 'P', 'L', 'O', 'O', 'P', '1' };
    constexpr const char* loopTag = "Loop";

    // The switch parameters are momentary: they are never saved or restored as "pressed".
    constexpr const char* switchIds[] = { "main", "stop", "undo", "clear" };

    // FLAC compression level for the loop stored in the session. Higher levels
    // are much slower and only a few percent smaller.
    constexpr int flacLevel = 3;

    constexpr double minRate = 8000.0, maxRate = 768000.0;

    // The Time Signature choices (used when the host doesn't give one).
    constexpr int timeSignatures[][2] = { { 4, 4 }, { 3, 4 }, { 2, 4 }, { 5, 4 }, { 6, 8 }, { 7, 8 }, { 9, 8 }, { 12, 8 } };

    void releaseSwitches (juce::ValueTree& state)
    {
        for (auto child : state)
            for (const auto* id : switchIds)
                if (child.getProperty ("id").toString() == id)
                    child.setProperty ("value", 0.0f, nullptr);
    }

    juce::String percentText (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }

    std::unique_ptr<juce::AudioParameterFloat> percent (const char* id, const char* name, float maximum, float def)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.0f, maximum, 1.0f), def,
            juce::AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (percentText));
    }

    std::unique_ptr<juce::AudioParameterBool> toggle (const char* id, const char* name, bool def)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
    }

    std::unique_ptr<juce::AudioParameterChoice> choice (const char* id, const char* name, juce::StringArray items, int def)
    {
        return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, std::move (items), def);
    }

    juce::String describe (LooperEngine::Result result, const char* tooLongText)
    {
        switch (result)
        {
            case LooperEngine::Result::empty:   return "There is no loop yet";
            case LooperEngine::Result::busy:    return "Not while the loop is being recorded";
            case LooperEngine::Result::tooLong: return tooLongText;
            case LooperEngine::Result::ok:
            default:                            return {};
        }
    }
}

//==============================================================================
class LooperProcessor::Worker : public juce::Thread
{
public:
    explicit Worker (LooperProcessor& p) : Thread ("OAP Looper"), owner (p) {}
    void run() override { owner.workerLoop (*this); }

private:
    LooperProcessor& owner;
};

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout LooperProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (percent ("level", "Loop Level", 150.0f, 100.0f));
    layout.add (percent ("feedback", "Overdub Feedback", 100.0f, 100.0f));
    layout.add (toggle ("monitor", "Monitor", true));
    layout.add (choice ("afterrec", "After Recording", { "Play", "Overdub" }, 0));
    layout.add (choice ("stopmode", "Stop Mode", { "Instant", "Fade Out", "Loop End" }, 0));
    layout.add (choice ("speed", "Speed", { "Half", "Normal", "Double" }, 1));
    layout.add (toggle ("reverse", "Reverse", false));

    layout.add (toggle ("sync", "Tempo Sync", true));
    layout.add (choice ("quantize", "Quantize", { "Off", "Beat", "Bar" }, 2));
    layout.add (choice ("length", "Record Length",
                        { "Free", "1 Beat", "2 Beats", "1 Bar", "2 Bars", "4 Bars", "8 Bars", "16 Bars" }, 0));
    layout.add (toggle ("autostart", "Auto Start", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tempo", 1 }, "Tempo", juce::NormalisableRange<float> (40.0f, 240.0f, 0.1f), 120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("BPM")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })));

    // The guide rhythm ("click" keeps its old ID: it switches the rhythm on and off).
    layout.add (toggle ("click", "Rhythm", false));
    {
        juce::StringArray patterns;
        for (int i = 0; i < looper::RhythmGuide::numPatterns; ++i)
            patterns.add (looper::RhythmGuide::patternName (i));
        layout.add (choice ("rhythm", "Rhythm Pattern", patterns, 0));
    }
    layout.add (percent ("rhythmlevel", "Rhythm Level", 100.0f, 70.0f));
    {
        juce::StringArray meters;
        for (const auto& m : timeSignatures)
            meters.add (juce::String (m[0]) + "/" + juce::String (m[1]));
        layout.add (choice ("timesig", "Time Signature", meters, 0));
    }

    // Auto loop: one click counts in, records, adds layers and plays.
    layout.add (choice ("autoloop", "Auto Loop", { "Off", "1 Layer", "2 Layers", "3 Layers", "4 Layers" }, 0));
    layout.add (choice ("countin", "Count-in", { "Off", "1 Bar", "2 Bars" }, 1));

    // For foot controllers and automation: map these in the host.
    layout.add (choice ("switchmode", "Mapped Switches", { "Momentary", "Latching" }, 0));
    layout.add (toggle ("main", "Main Switch", false));
    layout.add (toggle ("stop", "Stop Switch", false));
    layout.add (toggle ("undo", "Undo Switch", false));
    layout.add (toggle ("clear", "Clear Switch", false));

    return layout;
}

LooperProcessor::LooperProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createLayout())
{
    params.level       = parameters.getRawParameterValue ("level");
    params.feedback    = parameters.getRawParameterValue ("feedback");
    params.monitor     = parameters.getRawParameterValue ("monitor");
    params.afterRecord = parameters.getRawParameterValue ("afterrec");
    params.stopMode    = parameters.getRawParameterValue ("stopmode");
    params.speed       = parameters.getRawParameterValue ("speed");
    params.reverse     = parameters.getRawParameterValue ("reverse");
    params.sync        = parameters.getRawParameterValue ("sync");
    params.quantize    = parameters.getRawParameterValue ("quantize");
    params.length      = parameters.getRawParameterValue ("length");
    params.autoStart   = parameters.getRawParameterValue ("autostart");
    params.click       = parameters.getRawParameterValue ("click");
    params.rhythm      = parameters.getRawParameterValue ("rhythm");
    params.rhythmLevel = parameters.getRawParameterValue ("rhythmlevel");
    params.timeSig     = parameters.getRawParameterValue ("timesig");
    params.autoLoop    = parameters.getRawParameterValue ("autoloop");
    params.countIn     = parameters.getRawParameterValue ("countin");
    params.tempo       = parameters.getRawParameterValue ("tempo");
    params.switchMode  = parameters.getRawParameterValue ("switchmode");
    params.mainSwitch  = parameters.getRawParameterValue ("main");
    params.stopSwitch  = parameters.getRawParameterValue ("stop");
    params.undoSwitch  = parameters.getRawParameterValue ("undo");
    params.clearSwitch = parameters.getRawParameterValue ("clear");

    currentRate = engine.getSampleRate();

    worker = std::make_unique<Worker> (*this);
    worker->startThread (juce::Thread::Priority::low);
    startTimerHz (20);
}

LooperProcessor::~LooperProcessor()
{
    stopTimer();
    worker->stopThread (10000);
}

//==============================================================================
double LooperProcessor::getTailLengthSeconds() const
{
    // A loop keeps playing without any input. Audio Units need a finite number.
    if (wrapperType == wrapperType_AudioUnit || wrapperType == wrapperType_AudioUnitv3)
        return 3600.0;

    return std::numeric_limits<double>::infinity();
}

bool LooperProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    const auto ok = [] (const juce::AudioChannelSet& s)
    {
        return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo();
    };
    return ok (in) && ok (out) && in.size() <= out.size();
}

void LooperProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const std::lock_guard<std::mutex> refresh (refreshMutex);
    prepared = false;

    const double oldRate = engine.getSampleRate();

    if (std::abs (sampleRate - oldRate) > 1.0e-6)
    {
        // The sample rate has changed: take the loop along.
        LoopData loop;
        std::uint32_t generation = 0;
        const auto copied = engine.copyLoop (loop.left, loop.right, loop.bpm, generation, true);
        loop.sampleRate = oldRate;

        engine.prepare (sampleRate);
        currentRate = engine.getSampleRate();

        if (copied == LooperEngine::Result::ok)
            installLoop (loop, true);
    }
    else
    {
        engine.prepare (sampleRate);
    }

    scratch.setSize (1, juce::jmax (16, samplesPerBlock));
    numWaiting = 0;
    resyncSwitches = true;
    prepared = true;
}

//==============================================================================
void LooperProcessor::sendSwitch (Switch type) noexcept
{
    const auto now = juce::Time::getMillisecondCounter();
    switchFifo.write (1).forEach ([this, type, now] (int index) { switchQueue[(size_t) index] = { type, now }; });
}

void LooperProcessor::collectSwitches() noexcept
{
    const auto push = [this] (Switch type)
    {
        if (numWaiting < (int) waiting.size())
            waiting[(size_t) numWaiting++] = type;
    };

    // From the editor. A press made while audio wasn't running is forgotten, a release never is.
    const auto now = juce::Time::getMillisecondCounter();
    switchFifo.read (switchFifo.getNumReady()).forEach ([this, &push, now] (int index)
    {
        const auto& queued = switchQueue[(size_t) index];
        const bool release = queued.type == Switch::mainUp || queued.type == Switch::stopUp;
        if (release || now - queued.time < 1000u)
            push (queued.type);
    });

    // From the switch parameters (automation or a mapped foot controller).
    const bool main  = params.mainSwitch->load() > 0.5f;
    const bool stop  = params.stopSwitch->load() > 0.5f;
    const bool undo  = params.undoSwitch->load() > 0.5f;
    const bool clear = params.clearSwitch->load() > 0.5f;

    if (resyncSwitches.exchange (false))
    {
        lastMain = main;
        lastStop = stop;
        lastUndo = undo;
        lastClear = clear;
        return;
    }

    // Latching controllers flip the parameter with every press, so every change counts as a press.
    const bool latching = params.switchMode->load() > 0.5f;

    if (main != lastMain)
    {
        lastMain = main;
        if (latching || main)
            push (Switch::mainDown);
        if (latching || ! main)
            push (Switch::mainUp);
    }

    if (stop != lastStop)
    {
        lastStop = stop;
        if (latching || stop)
            push (Switch::stopDown);
        if (latching || ! stop)
            push (Switch::stopUp);
    }

    if (undo != lastUndo)
    {
        lastUndo = undo;
        if (latching || undo)
            push (Switch::undo);
    }

    if (clear != lastClear)
    {
        lastClear = clear;
        if (latching || clear)
            push (Switch::clear);
    }
}

void LooperProcessor::applySwitch (Switch type) noexcept
{
    switch (type)
    {
        case Switch::mainDown:  engine.mainSwitch (true); break;
        case Switch::mainUp:    engine.mainSwitch (false); break;
        case Switch::stopDown:  engine.stopSwitch (true); break;
        case Switch::stopUp:    engine.stopSwitch (false); break;
        case Switch::undo:      engine.undoSwitch(); break;
        case Switch::clear:     engine.clear(); break;
        default:                break;
    }
}

looper::Settings LooperProcessor::readSettings() const noexcept
{
    looper::Settings s;
    s.level = params.level->load() * 0.01f;
    s.feedback = params.feedback->load() * 0.01f;
    s.monitor = params.monitor->load() > 0.5f;
    s.overdubAfterRecord = params.afterRecord->load() > 0.5f;
    s.stopMode = (looper::StopMode) juce::jlimit (0, 2, juce::roundToInt (params.stopMode->load()));

    static constexpr float speeds[] = { 0.5f, 1.0f, 2.0f };
    s.speed = speeds[juce::jlimit (0, 2, juce::roundToInt (params.speed->load()))];
    s.reverse = params.reverse->load() > 0.5f;

    s.sync = params.sync->load() > 0.5f;
    s.quantize = (looper::Quantize) juce::jlimit (0, 2, juce::roundToInt (params.quantize->load()));

    // Free, 1 Beat, 2 Beats, 1 Bar, 2 Bars, 4 Bars, 8 Bars, 16 Bars
    static constexpr double beats[] = { 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    static constexpr double bars[]  = { 0.0, 0.0, 0.0, 1.0, 2.0, 4.0, 8.0, 16.0 };
    const int length = juce::jlimit (0, 7, juce::roundToInt (params.length->load()));
    s.fixedBeats = beats[length];
    s.fixedBars = bars[length];

    s.autoStart = params.autoStart->load() > 0.5f;
    s.click = params.click->load() > 0.5f;
    s.rhythm = juce::jlimit (0, looper::RhythmGuide::numPatterns - 1, juce::roundToInt (params.rhythm->load()));
    s.rhythmLevel = params.rhythmLevel->load() * 0.01f;

    const int autoLoop = juce::jlimit (0, 4, juce::roundToInt (params.autoLoop->load()));
    s.autoLoop = autoLoop > 0;
    s.autoLayers = juce::jmax (0, autoLoop - 1); // the layers after the first recording
    s.countInBars = juce::jlimit (0, 2, juce::roundToInt (params.countIn->load()));
    return s;
}

looper::Timing LooperProcessor::readTiming() noexcept
{
    looper::Timing t;
    t.bpm = (double) params.tempo->load();
    bool fromHost = false, meterFromHost = false;

    const auto& chosen = timeSignatures[juce::jlimit (0, (int) std::size (timeSignatures) - 1, juce::roundToInt (params.timeSig->load()))];
    t.numerator = chosen[0];
    t.denominator = chosen[1];
    t.beatsPerBar = 4.0 * chosen[0] / chosen[1];

    if (auto* head = getPlayHead())
    {
        if (const auto position = head->getPosition())
        {
            if (const auto bpm = position->getBpm())
            {
                if (*bpm > 0.0)
                {
                    t.bpm = *bpm;
                    fromHost = true;
                }
            }

            if (const auto signature = position->getTimeSignature())
            {
                if (signature->numerator > 0 && signature->denominator > 0)
                {
                    t.numerator = signature->numerator;
                    t.denominator = signature->denominator;
                    t.beatsPerBar = 4.0 * (double) signature->numerator / (double) signature->denominator;
                    meterFromHost = true;
                }
            }

            if (position->getIsPlaying())
            {
                if (const auto beat = position->getPpqPosition())
                {
                    t.hostPlaying = true;
                    t.hostBeat = *beat;

                    if (const auto bar = position->getPpqPositionOfLastBarStart())
                        t.hostBarStart = *bar;
                    else
                        t.hostBarStart = t.beatsPerBar * std::floor (*beat / t.beatsPerBar);
                }
            }
        }
    }

    hostTempo.store (fromHost, std::memory_order_relaxed);
    hostMeter.store (meterFromHost, std::memory_order_relaxed);
    hostPlaying.store (t.hostPlaying, std::memory_order_relaxed);
    return t;
}

void LooperProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (numCh == 0)
        return;

    // Input: never let a broken signal into the loop, and feed both sides from a mono input.
    float peak = 0.0f;
    for (int ch = 0; ch < juce::jmin (numIn, numCh); ++ch)
    {
        const auto range = juce::FloatVectorOperations::findMinAndMax (buffer.getReadPointer (ch), n);
        if (! std::isfinite (range.getStart()) || ! std::isfinite (range.getEnd()))
            buffer.clear (ch, 0, n);
        else
            peak = juce::jmax (peak, -range.getStart(), range.getEnd());
    }

    if (numIn == 0)
        buffer.clear (0, 0, n);
    for (int ch = juce::jmax (1, numIn); ch < buffer.getNumChannels(); ++ch)
    {
        if (ch < numCh)
            buffer.copyFrom (ch, 0, buffer, 0, 0, n);
        else
            buffer.clear (ch, 0, n);
    }

    if (peak > inputPeak.load (std::memory_order_relaxed))
        inputPeak.store (peak, std::memory_order_relaxed);

    // Controls, tempo and switches.
    collectSwitches();
    engine.beginBlock (readSettings(), readTiming());

    if (cancelRecording.load (std::memory_order_relaxed) && engine.acceptsEvents())
    {
        engine.clear(); // a loop is waiting to be loaded (see installLoop)
        cancelRecording = false;
    }

    if (numWaiting > 0)
    {
        if (engine.acceptsEvents())
        {
            for (int i = 0; i < numWaiting; ++i)
                applySwitch (waiting[(size_t) i]);
            numWaiting = 0;
        }
        else
        {
            engine.requestAbort(); // another thread is copying the loop: it gives way, the switches follow with the next block
        }
    }

    if (n == 0)
        return;

    float* left = buffer.getWritePointer (0);

    if (numCh > 1)
    {
        engine.process (left, buffer.getWritePointer (1), n);
    }
    else
    {
        // Mono output: the loop can still be stereo (a loaded file), so play both sides and mix them.
        float* right = scratch.getWritePointer (0);
        for (int start = 0; start < n; start += scratch.getNumSamples())
        {
            const int len = juce::jmin (scratch.getNumSamples(), n - start);
            juce::FloatVectorOperations::copy (right, left + start, len);
            engine.process (left + start, right, len);
            for (int i = 0; i < len; ++i)
                left[start + i] = 0.5f * (left[start + i] + right[i]);
        }
    }

    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto range = juce::FloatVectorOperations::findMinAndMax (buffer.getReadPointer (ch), n);
        if (! std::isfinite (range.getStart()) || ! std::isfinite (range.getEnd()))
        {
            buffer.clear();
            break;
        }
    }
}

//==============================================================================
void LooperProcessor::timerCallback()
{
    engine.serviceMemory();
}

void LooperProcessor::workerLoop (juce::Thread& thread)
{
    while (! thread.threadShouldExit())
    {
        engine.serviceMemory();

        const auto state = engine.getStatus().state;
        const bool recording = state == looper::State::Armed || state == looper::State::Recording
                            || state == looper::State::Overdubbing;
        if (! recording)
            refreshSnapshot (false);

        thread.wait (40);
    }
}

bool LooperProcessor::refreshSnapshot (bool wait)
{
    std::unique_lock<std::mutex> refresh (refreshMutex, std::defer_lock);
    if (wait)
        refresh.lock();
    else if (! refresh.try_lock())
        return false;

    {
        const std::lock_guard<std::mutex> lock (snapshotMutex);
        if (snapshot.valid && snapshot.generation == engine.getGeneration())
            return true;
    }

    Snapshot fresh;
    auto result = LooperEngine::Result::busy;

    for (int attempt = 0; attempt < 3; ++attempt)
    {
        fresh.loopId = engine.getLoopId();
        result = engine.copyLoop (copyLeft, copyRight, fresh.bpm, fresh.generation, ! prepared.load());

        if (result == LooperEngine::Result::busy)
            return false;
        if (fresh.loopId == engine.getLoopId())
            break; // otherwise the loop was replaced while it was being copied: copy the new one
        if (attempt == 2)
            return false;
    }

    fresh.valid = true;
    fresh.sampleRate = currentRate.load();

    if (result == LooperEngine::Result::ok)
    {
        fresh.numSamples = (int) juce::jmin (copyLeft.size(), copyRight.size());
        looper::makeOverview (copyLeft.data(), copyRight.data(), fresh.numSamples, overviewSize, fresh.peaks);

        if (! encodeLoop (copyLeft, copyRight, fresh))
            fresh.audio.reset(); // the loop can't be saved, but it can still be shown
    }

    // Don't hold on to a lot of memory after a long loop.
    if (copyLeft.capacity() > (size_t) 4 << 20)
    {
        std::vector<float>().swap (copyLeft);
        std::vector<float>().swap (copyRight);
    }

    const auto generation = fresh.generation;
    {
        const std::lock_guard<std::mutex> lock (snapshotMutex);
        snapshot = std::move (fresh);
    }
    overviewSerial.fetch_add (1);

    return generation == engine.getGeneration();
}

void LooperProcessor::copyOverview (std::vector<float>& peaks, double& lengthSeconds, std::uint32_t& generation)
{
    const std::lock_guard<std::mutex> lock (snapshotMutex);
    peaks = snapshot.peaks;
    lengthSeconds = snapshot.sampleRate > 0.0 ? (double) snapshot.numSamples / snapshot.sampleRate : 0.0;
    generation = snapshot.generation;
}

//==============================================================================
bool LooperProcessor::encodeLoop (const std::vector<float>& left, const std::vector<float>& right, Snapshot& s)
{
    const int n = s.numSamples;
    if (n <= 0)
        return false;

    const bool mono = std::memcmp (left.data(), right.data(), (size_t) n * sizeof (float)) == 0;
    s.channels = mono ? 1 : 2;

    // FLAC stores 24-bit whole numbers, so overdubs that went above full scale are turned down to
    // fit - by a power of two, which is exact: a loop that is saved, restored and saved again
    // stays bit for bit the same.
    const auto rangeLeft = juce::FloatVectorOperations::findMinAndMax (left.data(), n);
    const auto rangeRight = juce::FloatVectorOperations::findMinAndMax (right.data(), n);
    const float peak = juce::jmax (-rangeLeft.getStart(), rangeLeft.getEnd(), -rangeRight.getStart(), rangeRight.getEnd());
    if (! std::isfinite (peak) || peak > 1.0e6f)
        return false;

    s.scale = 1.0f;
    while (peak >= s.scale)
        s.scale *= 2.0f;

    s.audio.reset();
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::MemoryOutputStream> (s.audio, false);

    // The real sample rate is stored next to the audio; FLAC itself only accepts a few standard rates.
    juce::FlacAudioFormat format;
    auto writer = format.createWriterFor (stream, juce::AudioFormatWriterOptions()
                                                      .withSampleRate (48000.0)
                                                      .withNumChannels (s.channels)
                                                      .withBitsPerSample (24)
                                                      .withQualityOptionIndex (flacLevel));
    if (writer == nullptr)
        return false;

    constexpr int chunk = 16384;
    constexpr int fullScale = 8388608; // 2^23
    std::vector<int> converted[2];
    for (auto& v : converted)
        v.resize ((size_t) chunk);

    const float gain = (float) fullScale / s.scale;
    const float* sources[2] = { left.data(), right.data() };

    for (int start = 0; start < n; start += chunk)
    {
        const int len = juce::jmin (chunk, n - start);

        for (int ch = 0; ch < s.channels; ++ch)
        {
            const float* source = sources[ch] + start;
            int* dest = converted[ch].data();

            // (the writer wants the 24 bits at the top of 32)
            for (int i = 0; i < len; ++i)
                dest[i] = juce::jlimit (-fullScale, fullScale - 1, juce::roundToInt (source[i] * gain)) * 256;
        }

        const int* channels[3] = { converted[0].data(), s.channels > 1 ? converted[1].data() : nullptr, nullptr };
        if (! writer->write (channels, len))
            return false;
    }

    writer.reset(); // finishes the stream
    return s.audio.getSize() > 0;
}

bool LooperProcessor::decodeLoop (const void* data, size_t size, int numSamples, float scale, LoopData& loop)
{
    juce::FlacAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader (format.createReaderFor (new juce::MemoryInputStream (data, size, false), true));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->numChannels == 0)
        return false;

    auto n = reader->lengthInSamples;
    if (numSamples > 0)
        n = juce::jmin (n, (juce::int64) numSamples);
    n = juce::jmin (n, (juce::int64) (LooperEngine::maxSeconds * maxRate));

    loop.left.assign ((size_t) n, 0.0f);
    loop.right.assign ((size_t) n, 0.0f);
    float* channels[2] = { loop.left.data(), loop.right.data() };
    juce::AudioBuffer<float> view (channels, 2, (int) n);

    if (! reader->read (&view, 0, (int) n, 0, true, true))
        return false;

    if (std::isfinite (scale) && scale > 1.0f && scale <= 1.0e6f)
        view.applyGain (scale);

    return true;
}

LooperEngine::Result LooperProcessor::installLoop (LoopData& loop, bool audioStopped)
{
    const double rate = currentRate.load();
    auto n = juce::jmin (loop.left.size(), loop.right.size());
    bool truncated = false;

    if (n > 0 && std::abs (loop.sampleRate - rate) > 1.0e-6)
    {
        const auto longest = (size_t) (LooperEngine::maxSeconds * loop.sampleRate);
        if (n > longest)
        {
            n = longest;
            truncated = true;
        }

        loop.left.resize (n);
        loop.right.resize (n);
        looper::resampleLoop (loop.left, loop.right, loop.sampleRate, rate, (int) (LooperEngine::maxSeconds * rate));
        loop.sampleRate = rate;
        n = juce::jmin (loop.left.size(), loop.right.size());
    }

    auto result = LooperEngine::Result::busy;

    for (int attempt = 0; attempt < 20; ++attempt)
    {
        result = engine.loadLoop (n > 0 ? loop.left.data() : nullptr, n > 0 ? loop.right.data() : nullptr,
                                  (int) n, loop.bpm, audioStopped);
        if (result != LooperEngine::Result::busy)
            break;

        // Something is being recorded: ask the audio thread to cancel it, and wait until it has.
        // The request is withdrawn before the next attempt, so it can never clear the new loop.
        cancelRecording = true;
        for (int i = 0; i < 60 && cancelRecording.load(); ++i)
            juce::Thread::sleep (1);
        cancelRecording = false;
    }

    return (truncated && result == LooperEngine::Result::ok) ? LooperEngine::Result::tooLong : result;
}

//==============================================================================
LooperProcessor::Outcome LooperProcessor::saveLoopToFile (const juce::File& chosen)
{
    std::vector<float> left, right;
    double bpm = 0.0, rate = 0.0;

    {
        const std::lock_guard<std::mutex> refresh (refreshMutex);
        std::uint32_t generation = 0;
        const auto result = engine.copyLoop (left, right, bpm, generation, ! prepared.load());
        rate = currentRate.load();

        if (result == LooperEngine::Result::empty)
            return { false, "There is no loop to save" };
        if (result != LooperEngine::Result::ok)
            return { false, "Finish recording before saving the loop" };
    }

    const auto file = chosen.hasFileExtension ("wav") ? chosen : chosen.withFileExtension ("wav");
    const int n = (int) juce::jmin (left.size(), right.size());
    const auto failure = Outcome { false, "Couldn't write " + file.getFileName() };

    juce::TemporaryFile temp (file);
    {
        std::unique_ptr<juce::OutputStream> stream (temp.getFile().createOutputStream());
        if (stream == nullptr)
            return failure;

        auto options = juce::AudioFormatWriterOptions()
                           .withSampleRate (rate)
                           .withNumChannels (2)
                           .withBitsPerSample (32)
                           .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);

        if (bpm > 0.0)
        {
            // The loop's tempo travels with the file, so it can follow the tempo again when it is loaded.
            const double beats = (double) n / rate * bpm / 60.0;
            options = options.withMetadataValues ({ { juce::WavAudioFormat::acidizerFlag, "1" },
                                                    { juce::WavAudioFormat::acidTempo, juce::String (bpm, 3) },
                                                    { juce::WavAudioFormat::acidBeats, juce::String (juce::jmax (1, juce::roundToInt (beats))) },
                                                    { juce::WavAudioFormat::acidNumerator, "4" },
                                                    { juce::WavAudioFormat::acidDenominator, "4" } });
        }

        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (stream, options);
        if (writer == nullptr)
            return failure;

        const float* channels[2] = { left.data(), right.data() };
        if (! writer->writeFromFloatArrays (channels, 2, n))
            return failure;
    }

    if (! temp.overwriteTargetFileWithTemporary())
        return failure;

    return { true, "Saved " + file.getFileName() };
}

LooperProcessor::Outcome LooperProcessor::loadLoopFromFile (const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
        return { false, "Can't read " + file.getFileName() };

    if (reader->sampleRate < minRate || reader->sampleRate > maxRate || reader->numChannels == 0)
        return { false, file.getFileName() + " has a format the looper can't use" };

    const auto longest = (juce::int64) (LooperEngine::maxSeconds * reader->sampleRate);
    const bool truncated = reader->lengthInSamples > longest;
    const auto n = juce::jmin (reader->lengthInSamples, longest);

    if ((double) n / reader->sampleRate < 0.11)
        return { false, file.getFileName() + " is too short to loop" };

    LoopData loop;
    loop.sampleRate = reader->sampleRate;
    loop.left.assign ((size_t) n, 0.0f);
    loop.right.assign ((size_t) n, 0.0f);

    {
        float* channels[2] = { loop.left.data(), loop.right.data() };
        juce::AudioBuffer<float> view (channels, 2, (int) n);
        if (! reader->read (&view, 0, (int) n, 0, true, true))
            return { false, "Can't read " + file.getFileName() };
    }

    for (auto* channel : { &loop.left, &loop.right })
        for (auto& sample : *channel)
            if (! std::isfinite (sample))
                sample = 0.0f;

    // Loops that carry their tempo (ours, and most sample-pack loops) can follow the tempo when Sync is on.
    const auto& meta = reader->metadataValues;
    const double tempo = meta.getValue (juce::WavAudioFormat::acidTempo, "0").getDoubleValue();
    if (tempo >= 20.0 && tempo <= 400.0 && meta.getValue (juce::WavAudioFormat::acidOneShot, "0") != "1")
        loop.bpm = tempo;

    reader.reset();

    const std::lock_guard<std::mutex> refresh (refreshMutex);
    const auto result = installLoop (loop, ! prepared.load());

    if (result == LooperEngine::Result::busy)
        return { false, "The looper is busy - stop it and try again" };
    if (result == LooperEngine::Result::tooLong || truncated)
        return { true, "Loaded the first 5 minutes of " + file.getFileName() };

    return { true, "Loaded " + file.getFileName() };
}

LooperProcessor::Outcome LooperProcessor::doubleLoop()
{
    const auto result = engine.doubleLength (! prepared.load());
    if (result == LooperEngine::Result::ok)
        return { true, "Loop doubled" };
    return { false, describe (result, "The loop can't be longer than 5 minutes") };
}

LooperProcessor::Outcome LooperProcessor::halveLoop()
{
    const auto result = engine.halveLength (! prepared.load());
    if (result == LooperEngine::Result::ok)
        return { true, "Loop halved" };
    return { false, describe (result, "The loop is too short to halve") };
}

//==============================================================================
juce::AudioProcessorEditor* LooperProcessor::createEditor()
{
    return new ui::LooperEditor (*this);
}

// State: "OAPLOOP1", the size of the XML, the parameters as XML (with a <Loop>
// element describing the audio), the size of the audio, the loop as FLAC.
void LooperProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    releaseSwitches (state);
    auto xml = state.createXml();
    if (xml == nullptr)
        return;

    // While an overdub is being recorded the loop can't be copied; the last copy is stored
    // instead, as long as it is a copy of this loop and not of one that has been replaced.
    const bool current = refreshSnapshot (true);
    juce::MemoryBlock audio;

    {
        const std::lock_guard<std::mutex> lock (snapshotMutex);
        if (snapshot.valid && snapshot.numSamples > 0 && snapshot.audio.getSize() > 0
            && (current || snapshot.loopId == engine.getLoopId()))
        {
            auto* loop = xml->createNewChildElement (loopTag);
            loop->setAttribute ("rate", snapshot.sampleRate);
            loop->setAttribute ("bpm", snapshot.bpm);
            loop->setAttribute ("samples", snapshot.numSamples);
            loop->setAttribute ("channels", snapshot.channels);
            loop->setAttribute ("scale", (double) snapshot.scale);
            audio = snapshot.audio;
        }
    }

    const auto text = xml->toString (juce::XmlElement::TextFormat().singleLine());
    const auto utf8 = text.toUTF8();
    const auto textSize = (int) utf8.sizeInBytes() - 1; // without the terminating zero

    juce::MemoryOutputStream out (destData, false);
    out.write (stateMagic, sizeof (stateMagic));
    out.writeInt (textSize);
    out.write (utf8.getAddress(), (size_t) textSize);
    out.writeInt ((int) audio.getSize());
    out.write (audio.getData(), audio.getSize());
}

void LooperProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml;
    const auto* bytes = static_cast<const char*> (data);
    const void* audio = nullptr;
    size_t audioSize = 0;

    if (sizeInBytes >= 16 && std::memcmp (bytes, stateMagic, sizeof (stateMagic)) == 0)
    {
        juce::MemoryInputStream in (data, (size_t) sizeInBytes, false);
        in.skipNextBytes (sizeof (stateMagic));

        const int textSize = in.readInt();
        if (textSize <= 0 || (juce::int64) textSize > in.getNumBytesRemaining())
            return;

        xml = juce::parseXML (juce::String::fromUTF8 (bytes + in.getPosition(), textSize));
        in.skipNextBytes (textSize);

        if (in.getNumBytesRemaining() >= 4)
        {
            const int size = in.readInt();
            if (size > 0 && (juce::int64) size <= in.getNumBytesRemaining())
            {
                audio = bytes + in.getPosition();
                audioSize = (size_t) size;
            }
        }
    }
    else
    {
        xml = getXmlFromBinary (data, sizeInBytes); // parameters only
    }

    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    LoopData loop;

    if (auto* info = xml->getChildByName (loopTag))
    {
        const double rate = info->getDoubleAttribute ("rate");
        const int samples = info->getIntAttribute ("samples");
        const auto scale = (float) info->getDoubleAttribute ("scale", 1.0);

        if (audio != nullptr && rate >= minRate && rate <= maxRate && samples > 0
            && decodeLoop (audio, audioSize, samples, scale, loop))
        {
            loop.sampleRate = rate;
            loop.bpm = juce::jlimit (0.0, 400.0, info->getDoubleAttribute ("bpm"));
        }
        else
        {
            loop = {};
        }

        xml->removeChildElement (info, true);
    }

    auto state = juce::ValueTree::fromXml (*xml);
    releaseSwitches (state);
    parameters.replaceState (state);
    resyncSwitches = true;

    const std::lock_guard<std::mutex> refresh (refreshMutex);

    if (! prepared.load() && ! loop.left.empty() && std::abs (loop.sampleRate - currentRate.load()) > 1.0e-6)
    {
        // Audio hasn't started yet: adopt the loop's sample rate, so that the loop only has to be
        // converted if the host really runs at a different rate.
        engine.prepare (loop.sampleRate);
        currentRate = engine.getSampleRate();
    }

    installLoop (loop, ! prepared.load());
}

} // namespace oap
