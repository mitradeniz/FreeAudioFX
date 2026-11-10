// Tests for the plugin layer, where the DSP meets the host: the Looper's
// session state (the loop is saved with the project), sample-rate changes,
// audio files, switches mapped to parameters, host tempo, bus layouts and the
// editor. The plugin runs like it does in a DAW: its audio on one thread,
// everything else on another.
//
//   oap_plugin_tests

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "TestFramework.h"

// These tests compare samples for exact equality on purpose.
#if defined (__GNUC__) || defined (__clang__)
 #pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include "plugin/LooperProcessor.h"
#include "ui/LooperEditor.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <random>
#include <thread>
#include <vector>

using testing::check;
using testing::section;

namespace
{
using oap::LooperProcessor;
using oap::looper::State;
using Switch = LooperProcessor::Switch;

constexpr double twoPi = 6.283185307179586;

//==============================================================================
/** A host: runs the plugin's audio on its own thread, a good deal faster than
    real time, and can provide a tempo and a running transport. */
class Host : private juce::AudioPlayHead
{
public:
    explicit Host (LooperProcessor& p, double sampleRate = 48000.0, int numInputs = 2, int numOutputs = 2)
        : plugin (p), rate (sampleRate), numIn (numInputs), numOut (numOutputs)
    {
        plugin.setPlayConfigDetails (numIn, numOut, rate, blockSize);
        plugin.setPlayHead (this);
        plugin.prepareToPlay (rate, blockSize);
        thread = std::thread ([this] { run(); });
        waitSamples (4 * blockSize); // audio is running
    }

    ~Host() override { stop(); }

    void stop()
    {
        if (! thread.joinable())
            return;

        quit = true;
        thread.join();
        plugin.releaseResources();
        plugin.setPlayHead (nullptr);
    }

    //==============================================================================
    std::atomic<float> level { 0.0f };      // input: two tones, the right channel different from the left
    std::atomic<double> bpm { 0.0 };        // 0 = the host gives no tempo
    std::atomic<bool> finite { true };
    std::atomic<float> peakLeft { 0.0f }, peakRight { 0.0f }; // output, since the last resetPeaks()

    void startTransport()
    {
        transportStart = samples.load();
        transport = true;
    }

    void resetPeaks()
    {
        waitSamples (2 * blockSize);
        peakLeft = 0.0f;
        peakRight = 0.0f;
    }

    void waitSamples (std::int64_t count)
    {
        const auto target = samples.load() + count;
        while (samples.load() < target)
            std::this_thread::sleep_for (std::chrono::microseconds (300));
    }

    void waitSeconds (double seconds) { waitSamples ((std::int64_t) std::llround (seconds * rate)); }

    /** Waits (in real time) until something is true. */
    bool waitFor (const std::function<bool()>& condition, int timeoutMs = 6000) const
    {
        const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds (timeoutMs);
        while (! condition())
        {
            if (std::chrono::steady_clock::now() > until)
                return false;
            std::this_thread::sleep_for (std::chrono::milliseconds (1));
        }
        return true;
    }

    bool waitForState (State wanted) const
    {
        return waitFor ([this, wanted] { return plugin.getStatus().state == wanted; });
    }

    /** Waits until the plugin's background thread has taken its copy of the current loop. */
    bool waitForSnapshot (int timeoutMs = 6000) const
    {
        return waitFor ([this]
        {
            std::vector<float> peaks;
            double length = 0.0;
            std::uint32_t generation = 0;
            plugin.copyOverview (peaks, length, generation);
            return generation == plugin.getStatus().generation;
        }, timeoutMs);
    }

    void tap()
    {
        plugin.sendSwitch (Switch::mainDown);
        plugin.sendSwitch (Switch::mainUp);
    }

    void set (const char* id, float value) const
    {
        if (auto* parameter = plugin.getParameters().getParameter (id))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }

    /** Lets enough time pass for the next click not to count as a double click. */
    void settle() { waitSeconds (0.5); }

    /** Records a loop of about this length and leaves it playing, with silence at the input. */
    bool recordLoop (double seconds, float inputLevel = 0.5f)
    {
        level = inputLevel;
        tap();
        const bool started = waitForState (State::Recording);
        waitSeconds (seconds);
        tap();
        const bool playing = waitForState (State::Playing);
        level = 0.0f;
        settle();
        return started && playing;
    }

    LooperProcessor& plugin;
    const double rate;
    static constexpr int blockSize = 256;

private:
    void run()
    {
        juce::AudioBuffer<float> buffer (juce::jmax (numIn, numOut), blockSize);
        juce::MidiBuffer midi;

        while (! quit.load())
        {
            const auto start = samples.load();
            const float a = level.load();

            for (int i = 0; i < blockSize; ++i)
            {
                const double t = (double) (start + i) / rate;
                const auto x = a * (float) (0.6 * std::sin (twoPi * 220.0 * t) + 0.4 * std::sin (twoPi * 331.0 * t + 1.0));
                buffer.setSample (0, i, x);
                if (buffer.getNumChannels() > 1)
                    buffer.setSample (1, i, numIn > 1 ? -0.5f * x : 0.0f);
            }

            plugin.processBlock (buffer, midi);

            for (int ch = 0; ch < numOut; ++ch)
            {
                const auto range = juce::FloatVectorOperations::findMinAndMax (buffer.getReadPointer (ch), blockSize);
                if (! std::isfinite (range.getStart()) || ! std::isfinite (range.getEnd()))
                    finite = false;

                auto& peak = ch == 0 ? peakLeft : peakRight;
                peak = juce::jmax (peak.load(), -range.getStart(), range.getEnd());
            }

            samples += blockSize;
            std::this_thread::sleep_for (std::chrono::microseconds (100));
        }
    }

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        const double tempo = bpm.load();

        if (tempo > 0.0)
        {
            info.setBpm (tempo);
            info.setTimeSignature (TimeSignature { 4, 4 });

            if (transport.load())
            {
                const double beat = (double) (samples.load() - transportStart.load()) / rate * tempo / 60.0;
                info.setIsPlaying (true);
                info.setPpqPosition (beat);
                info.setPpqPositionOfLastBarStart (4.0 * std::floor (beat / 4.0));
            }
        }

        return info;
    }

    const int numIn, numOut;
    std::atomic<std::int64_t> samples { 0 }, transportStart { 0 };
    std::atomic<bool> quit { false }, transport { false };
    std::thread thread;
};

//==============================================================================
struct Audio
{
    std::vector<float> left, right;
    double rate = 0.0, tempo = 0.0;
};

Audio readAudio (const juce::File& file)
{
    Audio audio;
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    if (std::unique_ptr<juce::AudioFormatReader> reader { formats.createReaderFor (file) })
    {
        const auto n = (int) reader->lengthInSamples;
        audio.left.assign ((size_t) n, 0.0f);
        audio.right.assign ((size_t) n, 0.0f);
        float* channels[2] = { audio.left.data(), audio.right.data() };
        juce::AudioBuffer<float> view (channels, 2, n);
        reader->read (&view, 0, n, 0, true, true);
        audio.rate = reader->sampleRate;
        audio.tempo = reader->metadataValues.getValue (juce::WavAudioFormat::acidTempo, "0").getDoubleValue();
    }

    return audio;
}

bool writeWav (const juce::File& file, const std::vector<float>& mono, double rate, int bits)
{
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (rate).withNumChannels (1).withBitsPerSample (bits));
    if (writer == nullptr)
        return false;

    const float* channels[1] = { mono.data() };
    return writer->writeFromFloatArrays (channels, 1, (int) mono.size());
}

double worstDifference (const std::vector<float>& a, const std::vector<float>& b)
{
    if (a.size() != b.size())
        return 1.0e9;

    double worst = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        worst = std::max (worst, (double) std::abs (a[i] - b[i]));
    return worst;
}

double rms (const std::vector<float>& v)
{
    double sum = 0.0;
    for (float x : v)
        sum += (double) x * (double) x;
    return v.empty() ? 0.0 : std::sqrt (sum / (double) v.size());
}

float parameterValue (LooperProcessor& plugin, const char* id)
{
    return plugin.getParameters().getRawParameterValue (id)->load();
}

/** The loop a plugin holds, by way of its "save as WAV". */
Audio loopOf (LooperProcessor& plugin, const juce::File& dir, const char* name)
{
    const auto file = dir.getChildFile (name);
    file.deleteFile();
    const auto outcome = plugin.saveLoopToFile (file);
    return outcome.ok ? readAudio (file.withFileExtension ("wav")) : Audio();
}

//==============================================================================
void testSessionState (const juce::File& dir)
{
    section ("Looper plugin: the loop is saved with the session");

    LooperProcessor a;
    Host hostA (a);
    check (hostA.recordLoop (1.0), "record a loop");

    // a second layer
    hostA.level = 0.3f;
    hostA.tap();
    hostA.waitSeconds (0.5);
    hostA.tap();
    hostA.level = 0.0f;
    check (hostA.waitFor ([&a] { return a.getStatus().layers == 2 && a.getStatus().canUndo; }), "overdub a second layer");
    hostA.waitSeconds (0.7);
    hostA.set ("level", 70.0f);

    const auto lengthA = a.getStatus().lengthSeconds;
    juce::MemoryBlock state;
    a.getStateInformation (state);
    const auto loopA = loopOf (a, dir, "a.wav");
    check (! loopA.left.empty() && std::abs ((double) loopA.left.size() / loopA.rate - lengthA) < 1.0e-9, "save as WAV writes the whole loop");
    std::printf ("  %.2f s stereo loop: %d bytes of session state (the 32-bit WAV has %d)\n", lengthA, (int) state.getSize(),
                 (int) dir.getChildFile ("a.wav").getSize());
    check (state.getSize() > 2000 && state.getSize() < (size_t) dir.getChildFile ("a.wav").getSize(), "the state holds the loop, compressed");
    check (a.getStatus().state == State::Playing, "saving doesn't interrupt playback");

    // A new instance, restored before audio starts - like opening a project.
    LooperProcessor b;
    b.setStateInformation (state.getData(), (int) state.getSize());
    {
        Host hostB (b);
        hostB.waitSeconds (0.05);
        const auto st = b.getStatus();
        check (st.state == State::Stopped && std::abs (st.lengthSeconds - lengthA) < 1.0e-9 && st.layers == 1,
               "the restored loop is there, stopped and ready to play");
        check (std::abs (parameterValue (b, "level") - 70.0f) < 1.0e-4f, "the parameters are restored too");

        const auto loopB = loopOf (b, dir, "b.wav");
        const double error = std::max (worstDifference (loopA.left, loopB.left), worstDifference (loopA.right, loopB.right));
        std::printf ("  restored loop differs by at most %.1f dB\n", testing::db (error));
        check (error < 2.5e-7, "the restored loop is the saved one (to 24-bit precision)");

        hostB.resetPeaks();
        hostB.tap();
        check (hostB.waitForState (State::Playing), "one click plays it");
        hostB.waitSeconds (0.5);
        check (hostB.peakLeft.load() > 0.05f && hostB.peakRight.load() > 0.02f && hostB.finite.load(), "and it is audible on both sides");

        // Restoring into a running instance replaces what it has (here: two layers, playing).
        juce::MemoryBlock stateB;
        b.getStateInformation (stateB);
        a.setStateInformation (stateB.getData(), (int) stateB.getSize());
        check (hostA.waitFor ([&a, lengthA]
                              {
                                  const auto now = a.getStatus();
                                  return now.state == State::Stopped && now.layers == 1 && std::abs (now.lengthSeconds - lengthA) < 1.0e-9;
                              }),
               "restoring while audio runs replaces the loop");
    }

    // Saved again and again, the loop doesn't degrade.
    juce::MemoryBlock second;
    b.getStateInformation (second);
    LooperProcessor c;
    c.setStateInformation (second.getData(), (int) second.getSize());
    const auto loopC = loopOf (c, dir, "c.wav");
    const auto loopB = readAudio (dir.getChildFile ("b.wav"));
    check (worstDifference (loopB.left, loopC.left) == 0.0 && worstDifference (loopB.right, loopC.right) == 0.0,
           "saving a restored loop again changes nothing");
}

//==============================================================================
void testStateEdgeCases (const juce::File& dir)
{
    section ("Looper plugin: saving at awkward moments, and broken states");

    LooperProcessor a;
    Host host (a);

    // while the first recording is still running
    host.level = 0.5f;
    host.tap();
    check (host.waitForState (State::Recording), "(recording)");
    host.waitSeconds (0.3);
    juce::MemoryBlock whileRecording;
    a.getStateInformation (whileRecording);
    check (a.getStatus().state == State::Recording, "saving doesn't disturb a recording");
    host.waitSeconds (0.5);
    host.tap();
    check (host.waitForState (State::Playing), "(playing)");
    host.level = 0.0f;
    host.settle();
    const auto length = a.getStatus().lengthSeconds;

    LooperProcessor b;
    b.setStateInformation (whileRecording.getData(), (int) whileRecording.getSize());
    check (b.getStatus().state == State::Empty, "a state saved during the first recording has no loop yet");

    // while an overdub is running: the loop as it was before
    check (host.waitForSnapshot(), "(the background copy is taken)");
    const auto before = loopOf (a, dir, "before.wav");
    host.level = 0.4f;
    host.tap();
    check (host.waitForState (State::Overdubbing), "(overdubbing)");
    host.waitSeconds (0.3);
    juce::MemoryBlock whileOverdubbing;
    a.getStateInformation (whileOverdubbing);
    check (a.getStatus().state == State::Overdubbing, "saving doesn't disturb an overdub");
    host.settle();
    host.tap();
    check (host.waitForState (State::Playing) && a.getStatus().layers == 2, "(which then ends normally)");
    host.level = 0.0f;

    LooperProcessor c;
    c.setStateInformation (whileOverdubbing.getData(), (int) whileOverdubbing.getSize());
    const auto restored = loopOf (c, dir, "restored.wav");
    check (std::abs (c.getStatus().lengthSeconds - length) < 1.0e-9 && worstDifference (before.left, restored.left) < 2.5e-7,
           "a state saved during an overdub holds the loop as it was before the overdub");

    // parameters only (the plain format) clears the loop
    juce::MemoryBlock plain;
    if (auto xml = a.getParameters().copyState().createXml())
        juce::AudioProcessor::copyXmlToBinary (*xml, plain);
    c.setStateInformation (plain.getData(), (int) plain.getSize());
    check (c.getStatus().state == State::Empty, "a state without a loop leaves the looper empty");

    // rubbish
    std::mt19937 rng (7);
    bool survived = true;
    for (int size : { 0, 1, 7, 8, 15, 16, 17, 64, 1000, 20000 })
    {
        std::vector<char> junk ((size_t) size);
        for (auto& byte : junk)
            byte = (char) (rng() & 0xff);
        c.setStateInformation (junk.data(), size);

        if (size >= 16)
        {
            std::memcpy (junk.data(), "OAPLOOP1", 8); // our header, followed by nonsense
            c.setStateInformation (junk.data(), size);
        }
        survived = survived && c.getStatus().state == State::Empty;
    }
    for (size_t size : { whileOverdubbing.getSize() / 2, whileOverdubbing.getSize() - 1, (size_t) 40 })
        c.setStateInformation (whileOverdubbing.getData(), (int) size); // truncated
    survived = survived && (c.getStatus().state == State::Empty || c.getStatus().state == State::Stopped);
    check (survived, "random and truncated states are refused without harm");

    // the switch parameters are never restored as "pressed"
    LooperProcessor d;
    d.getParameters().getParameter ("main")->setValueNotifyingHost (1.0f);
    d.getParameters().getParameter ("clear")->setValueNotifyingHost (1.0f);
    juce::MemoryBlock pressed;
    d.getStateInformation (pressed);
    LooperProcessor e;
    e.setStateInformation (pressed.getData(), (int) pressed.getSize());
    {
        Host hostE (e);
        hostE.level = 0.5f;
        hostE.waitSeconds (0.3);
        check (parameterValue (e, "main") < 0.5f && parameterValue (e, "clear") < 0.5f && e.getStatus().state == State::Empty,
               "a state saved with a switch held down doesn't press it when restored");
    }

    // louder than full scale, and mono
    LooperProcessor loud;
    {
        Host hostLoud (loud);
        check (hostLoud.recordLoop (0.5, 2.5f), "(a loop that goes over full scale)");
        const auto original = loopOf (loud, dir, "loud.wav");
        juce::MemoryBlock state;
        loud.getStateInformation (state);
        LooperProcessor back;
        back.setStateInformation (state.getData(), (int) state.getSize());
        const auto copy = loopOf (back, dir, "loud2.wav");
        float peak = 0.0f;
        for (float v : original.left)
            peak = std::max (peak, std::abs (v));
        check (peak > 1.5f && worstDifference (original.left, copy.left) < 1.0e-6 && worstDifference (original.right, copy.right) < 1.0e-6,
               "a loop above full scale is saved without clipping");

        juce::MemoryBlock again;
        back.getStateInformation (again);
        LooperProcessor third;
        third.setStateInformation (again.getData(), (int) again.getSize());
        const auto copy2 = loopOf (third, dir, "loud3.wav");
        check (worstDifference (copy.left, copy2.left) == 0.0 && worstDifference (copy.right, copy2.right) == 0.0,
               "and doesn't change when it is saved again");
    }

    LooperProcessor mono;
    {
        Host hostMono (mono, 48000.0, 1, 2);
        check (hostMono.recordLoop (0.5), "(a loop from a mono input)");
        const auto original = loopOf (mono, dir, "mono.wav");
        juce::MemoryBlock state;
        mono.getStateInformation (state);
        LooperProcessor back;
        back.setStateInformation (state.getData(), (int) state.getSize());
        const auto copy = loopOf (back, dir, "mono2.wav");
        check (! copy.left.empty() && worstDifference (copy.left, copy.right) == 0.0 && worstDifference (original.left, copy.left) < 2.5e-7,
               "a mono loop comes back the same on both sides");
    }

    check (host.finite.load(), "audio stays finite throughout");
}

//==============================================================================
void testSampleRates (const juce::File& dir)
{
    section ("Looper plugin: sample-rate changes");

    LooperProcessor a;
    juce::MemoryBlock state;
    double length = 0.0, level = 0.0;
    {
        Host host (a, 48000.0);
        check (host.recordLoop (1.0), "(a loop at 48 kHz)");
        length = a.getStatus().lengthSeconds;
        a.getStateInformation (state);
        level = rms (loopOf (a, dir, "r48.wav").left);
    }

    // a project saved at 48 kHz, opened at 44.1 kHz
    LooperProcessor b;
    b.setStateInformation (state.getData(), (int) state.getSize());
    {
        Host host (b, 44100.0);
        host.waitSeconds (0.05);
        const auto loop = loopOf (b, dir, "r44.wav");
        check (std::abs (b.getStatus().lengthSeconds - length) < 1.0 / 44100.0 && std::abs (loop.rate - 44100.0) < 1.0e-9,
               "a loop saved at 48 kHz has the same duration at 44.1 kHz");
        check (std::abs (rms (loop.left) / level - 1.0) < 0.01, "and the same level");
    }

    // the audio device changes its rate while the plugin is loaded
    {
        Host host (a, 96000.0);
        host.waitSeconds (0.05);
        const auto st = a.getStatus();
        check (st.state == State::Stopped && std::abs (st.lengthSeconds - length) < 1.0 / 96000.0 && st.layers == 1,
               "switching the running plugin to 96 kHz keeps the loop");
        host.resetPeaks();
        host.tap();
        check (host.waitForState (State::Playing), "(plays)");
        host.waitSeconds (0.3);
        check (host.peakLeft.load() > 0.05f && host.finite.load(), "and it still plays");
    }
}

//==============================================================================
void testFiles (const juce::File& dir)
{
    section ("Looper plugin: audio files, double and halve");

    LooperProcessor a;
    Host host (a);

    check (! a.saveLoopToFile (dir.getChildFile ("nothing.wav")).ok && ! dir.getChildFile ("nothing.wav").existsAsFile(),
           "nothing to save when the looper is empty");
    check (! a.doubleLoop().ok && ! a.halveLoop().ok, "nothing to double or halve either");

    check (host.recordLoop (1.0), "(a loop)");
    const auto length = a.getStatus().lengthSeconds;
    const auto saved = loopOf (a, dir, "loop"); // no extension given
    check (dir.getChildFile ("loop.wav").existsAsFile() && ! saved.left.empty(), "save adds the .wav extension");
    check (std::abs (saved.tempo - 120.0) < 1.0e-6, "the file carries the loop's tempo");

    // load it into another instance: exactly the same loop, tempo included
    LooperProcessor b;
    {
        Host hostB (b);
        const auto outcome = b.loadLoopFromFile (dir.getChildFile ("loop.wav"));
        check (outcome.ok && hostB.waitForState (State::Stopped), "load a WAV file");
        const auto st = b.getStatus();
        check (std::abs (st.lengthSeconds - length) < 1.0e-9 && std::abs (st.loopBpm - 120.0) < 1.0e-6, "with its length and tempo");
        const auto again = loopOf (b, dir, "again.wav");
        check (worstDifference (saved.left, again.left) == 0.0 && worstDifference (saved.right, again.right) == 0.0,
               "the loaded loop is sample for sample the saved one");

        // a mono 16-bit file at another sample rate
        std::vector<float> tone ((size_t) 44100);
        for (size_t i = 0; i < tone.size(); ++i)
            tone[i] = (float) (0.4 * std::sin (twoPi * 441.0 * (double) i / 44100.0));
        check (writeWav (dir.getChildFile ("tone44.wav"), tone, 44100.0, 16), "(write a 44.1 kHz file)");
        check (b.loadLoopFromFile (dir.getChildFile ("tone44.wav")).ok
                   && hostB.waitFor ([&b] { return std::abs (b.getStatus().lengthSeconds - 1.0) < 1.0e-4; }),
               "a file at another sample rate is converted");
        const auto converted = loopOf (b, dir, "tone48.wav");
        check (converted.left.size() == 48000 && worstDifference (converted.left, converted.right) == 0.0
                   && std::abs (rms (converted.left) - 0.4 / std::sqrt (2.0)) < 0.002,
               "to the same duration and level, on both sides");
        check (std::abs (b.getStatus().loopBpm) < 1.0e-9, "a file without a tempo doesn't follow the tempo");

        // double and halve
        check (b.doubleLoop().ok && hostB.waitFor ([&b] { return std::abs (b.getStatus().lengthSeconds - 2.0) < 1.0e-4; }), "double the loop");
        check (b.halveLoop().ok && hostB.waitFor ([&b] { return std::abs (b.getStatus().lengthSeconds - 1.0) < 1.0e-4; }), "halve it again");

        // files the looper can't use
        std::vector<float> blip ((size_t) 2000, 0.1f);
        writeWav (dir.getChildFile ("short.wav"), blip, 48000.0, 24);
        dir.getChildFile ("text.wav").replaceWithText ("this is not audio");
        check (! b.loadLoopFromFile (dir.getChildFile ("short.wav")).ok && ! b.loadLoopFromFile (dir.getChildFile ("text.wav")).ok
                   && ! b.loadLoopFromFile (dir.getChildFile ("missing.wav")).ok,
               "files that are too short, not audio or missing are refused");
        hostB.waitSeconds (0.05);
        check (std::abs (b.getStatus().lengthSeconds - 1.0) < 1.0e-4, "and the loop stays as it was");

        // loading while a recording is running replaces it
        b.sendSwitch (Switch::clear);
        check (hostB.waitForState (State::Empty), "(clear)");
        hostB.level = 0.5f;
        hostB.tap();
        check (hostB.waitForState (State::Recording), "(recording)");
        check (b.loadLoopFromFile (dir.getChildFile ("loop.wav")).ok && hostB.waitForState (State::Stopped)
                   && std::abs (b.getStatus().lengthSeconds - length) < 1.0e-9,
               "loading during a recording cancels the recording and loads the file");
        check (hostB.finite.load(), "(finite)");
    }
}

//==============================================================================
void testMappedSwitches()
{
    section ("Looper plugin: switches mapped to parameters");

    LooperProcessor a;
    Host host (a);
    host.level = 0.4f;

    // momentary: the parameter is on while the switch is held
    host.set ("main", 1.0f);
    check (host.waitForState (State::Recording), "momentary: pressing the Main Switch parameter records");
    host.set ("main", 0.0f);
    host.waitSeconds (0.5);
    check (a.getStatus().state == State::Recording, "releasing it does nothing");
    host.set ("main", 1.0f);
    check (host.waitForState (State::Playing), "pressing again closes the loop");
    host.set ("main", 0.0f);
    host.waitSeconds (0.6);

    host.set ("stop", 1.0f);
    check (host.waitForState (State::Stopped), "the Stop Switch parameter stops");
    host.set ("stop", 0.0f);
    host.waitSeconds (0.6);

    host.set ("main", 1.0f); // held: starts playing, and after 0.8 s clears
    check (host.waitForState (State::Empty), "holding the Main Switch while stopped clears the loop");
    host.set ("main", 0.0f);
    host.waitSeconds (0.6);

    // latching: every change of the parameter is a press
    host.set ("switchmode", 1.0f);
    host.waitSeconds (0.05);
    host.set ("main", 1.0f);
    check (host.waitForState (State::Recording), "latching: switching the parameter on records");
    host.waitSeconds (0.5);
    host.set ("main", 0.0f);
    check (host.waitForState (State::Playing), "switching it off is the next press");
    host.waitSeconds (0.6);
    host.set ("main", 1.0f);
    check (host.waitForState (State::Overdubbing), "and on again the one after");
    host.set ("undo", 1.0f);
    check (host.waitForState (State::Playing), "the Undo Switch parameter cancels the overdub");
    host.waitSeconds (0.6);
    host.set ("clear", 1.0f);
    check (host.waitForState (State::Empty), "the Clear Switch parameter clears");
    host.waitSeconds (0.1);
    host.set ("undo", 0.0f);
    host.set ("clear", 0.0f);
    host.set ("stop", 1.0f);
    host.waitSeconds (0.2);
    check (a.getStatus().state == State::Empty && host.finite.load(), "switches on an empty looper do no harm");
}

//==============================================================================
void testHostTempo()
{
    section ("Looper plugin: host tempo and transport");

    LooperProcessor a;
    Host host (a);

    host.set ("tempo", 90.0f);
    host.waitSeconds (0.05);
    check (! a.isUsingHostTempo() && std::abs (a.getStatus().bpm - 90.0) < 1.0e-6, "without a host tempo the Tempo control is used");

    host.bpm = 100.0;
    host.waitSeconds (0.05);
    check (a.isUsingHostTempo() && ! a.isHostPlaying() && std::abs (a.getStatus().bpm - 100.0) < 1.0e-9, "the host's tempo takes over");

    // Sync is on by default: with the transport running, recording starts on the bar line
    // and a fixed length of one bar closes the loop by itself.
    host.set ("length", 3.0f); // 1 Bar
    host.startTransport();
    host.waitSeconds (0.3);
    check (a.isHostPlaying(), "the plugin sees the transport running");
    host.level = 0.5f;
    host.tap();
    host.waitSeconds (0.05);
    check (a.getStatus().state == State::Armed && a.getStatus().scheduled, "a click arms the looper until the next bar");
    check (host.waitForState (State::Recording), "recording starts by itself");
    check (std::abs (a.getStatus().targetSeconds - 2.4) < 1.0e-6, "one bar at 100 BPM is 2.4 s");
    check (host.waitForState (State::Playing), "and the loop closes by itself");
    const auto st = a.getStatus();
    check (std::abs (st.lengthSeconds - 2.4) < 1.0e-6 && std::abs (st.loopBpm - 100.0) < 1.0e-9, "the loop is exactly one bar long");
    host.level = 0.0f;

    // The loop follows the tempo: at 200 BPM it runs twice as fast.
    host.bpm = 200.0;
    host.waitSeconds (0.1);
    const double p0 = a.getStatus().positionSeconds;
    host.waitSeconds (0.25);
    double moved = a.getStatus().positionSeconds - p0;
    if (moved < 0.0)
        moved += 2.4;
    check (moved > 0.4 && moved < 0.62, "the loop follows a tempo change");
    check (host.finite.load(), "(finite)");
}

//==============================================================================
void testLayouts()
{
    section ("Looper plugin: mono and stereo");

    for (const auto& layout : { std::pair<int, int> { 1, 1 }, std::pair<int, int> { 1, 2 }, std::pair<int, int> { 2, 2 } })
    {
        LooperProcessor a;
        Host host (a, 48000.0, layout.first, layout.second);
        const bool recorded = host.recordLoop (0.5);
        host.resetPeaks();
        host.waitSeconds (0.6);
        const bool left = host.peakLeft.load() > 0.1f;
        const bool right = layout.second < 2 || host.peakRight.load() > 0.05f;
        check (recorded && left && right && host.finite.load(),
               std::to_string (layout.first) + " in, " + std::to_string (layout.second) + " out: the loop plays on every output");
    }

    LooperProcessor a;
    juce::AudioProcessor::BusesLayout stereoToMono;
    stereoToMono.inputBuses.add (juce::AudioChannelSet::stereo());
    stereoToMono.outputBuses.add (juce::AudioChannelSet::mono());
    check (! a.checkBusesLayoutSupported (stereoToMono), "stereo in, mono out is not offered");
}

//==============================================================================
void testEditor()
{
    section ("Looper plugin: editor");

    LooperProcessor a;
    Host host (a);
    std::unique_ptr<juce::AudioProcessorEditor> editor (a.createEditorAndMakeActive());
    auto* view = dynamic_cast<oap::ui::LooperEditor*> (editor.get());
    check (view != nullptr, "the editor opens");
    if (view == nullptr)
        return;

    const auto differentColours = [&editor, view]
    {
        view->refresh();
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 0.5f);
        std::vector<juce::uint32> seen;
        for (int y = 0; y < image.getHeight(); y += 3)
            for (int x = 0; x < image.getWidth(); x += 3)
            {
                const auto argb = image.getPixelAt (x, y).getARGB();
                if (std::find (seen.begin(), seen.end(), argb) == seen.end() && seen.size() < 2000)
                    seen.push_back (argb);
            }
        return (int) seen.size();
    };

    check (differentColours() > 100, "it draws itself when the looper is empty");

    host.level = 0.5f;
    host.tap();
    host.waitForState (State::Recording);
    for (int i = 0; i < 10; ++i)
    {
        host.waitSeconds (0.05);
        view->refresh();
    }
    check (differentColours() > 100, "while recording");

    host.tap();
    host.waitForState (State::Playing);
    host.waitForSnapshot();
    host.settle();
    host.tap();
    check (host.waitForState (State::Overdubbing), "(overdubbing)");
    for (int i = 0; i < 10; ++i)
    {
        host.waitSeconds (0.03);
        view->refresh();
    }
    check (differentColours() > 100, "while overdubbing");

    a.sendSwitch (Switch::clear);
    host.waitForState (State::Empty);
    view->refresh();

    a.editorBeingDeleted (editor.get());
    editor.reset();
    host.waitSeconds (0.1);
    check (host.finite.load(), "and closes again while audio runs");
}

//==============================================================================
void testLongLoop (const juce::File& dir)
{
    section ("Looper plugin: a long loop");

    // Two minutes of plucked notes with a little noise, the two sides different.
    constexpr int seconds = 120;
    std::vector<float> left ((size_t) seconds * 48000), right (left.size());
    std::mt19937 rng (3);
    std::uniform_real_distribution<float> noise (-0.0005f, 0.0005f);
    for (size_t i = 0; i < left.size(); ++i)
    {
        const double t = (double) i / 48000.0;
        const double note = std::floor (t * 3.0), inNote = t - note / 3.0;
        const double freq = 82.4 * std::pow (2.0, (double) ((int) note * 7 % 24) / 12.0);
        const double env = std::exp (-inNote * 4.0);
        left[i] = (float) (0.5 * env * (std::sin (twoPi * freq * inNote) + 0.3 * std::sin (twoPi * 2.0 * freq * inNote))) + noise (rng);
        right[i] = (float) (0.4 * env * std::sin (twoPi * freq * inNote + 0.5)) + noise (rng);
    }

    // as a 24-bit stereo WAV file
    const auto file = dir.getChildFile ("long.wav");
    {
        std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (48000.0).withNumChannels (2).withBitsPerSample (24));
        const float* channels[2] = { left.data(), right.data() };
        check (writer != nullptr && writer->writeFromFloatArrays (channels, 2, (int) left.size()), "(write a two-minute file)");
    }

    LooperProcessor a;
    Host host (a);
    const auto loadStart = std::chrono::steady_clock::now();
    check (a.loadLoopFromFile (file).ok && host.waitForState (State::Stopped), "load a two-minute loop");
    const auto loaded = std::chrono::steady_clock::now();
    check (host.waitForSnapshot (60000), "the background copy is ready");
    const auto ready = std::chrono::steady_clock::now();

    juce::MemoryBlock state;
    const auto saveStart = std::chrono::steady_clock::now();
    a.getStateInformation (state);
    const auto saved = std::chrono::steady_clock::now();
    const auto ms = [] (auto from, auto to) { return (int) std::chrono::duration_cast<std::chrono::milliseconds> (to - from).count(); };

    std::printf ("  loading %d ms, background copy %d ms, saving the session state %d ms\n", ms (loadStart, loaded), ms (loaded, ready),
                 ms (saveStart, saved));
    std::printf ("  session state %.1f MB (the 24-bit WAV has %.1f MB)\n", (double) state.getSize() / 1048576.0,
                 (double) file.getSize() / 1048576.0);
    check (state.getSize() < (size_t) file.getSize(), "the state is smaller than the WAV file");
    check (ms (saveStart, saved) < 500, "saving the state doesn't wait for the loop to be compressed again");

    LooperProcessor b;
    b.setStateInformation (state.getData(), (int) state.getSize());
    check (std::abs (b.getStatus().lengthSeconds - (double) seconds) < 1.0e-9, "and it restores to the whole loop");
    check (host.finite.load(), "(finite)");
}
} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                         .getChildFile ("oap_plugin_tests_" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt()));
    dir.createDirectory();

    testSessionState (dir);
    testStateEdgeCases (dir);
    testSampleRates (dir);
    testFiles (dir);
    testMappedSwitches();
    testHostTempo();
    testLayouts();
    testEditor();
    testLongLoop (dir);

    dir.deleteRecursively();

    std::printf ("\n%d checks, %d failures\n", testing::checks, testing::failures);
    return testing::failures == 0 ? 0 : 1;
}
