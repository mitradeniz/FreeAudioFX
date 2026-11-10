// Tests for the looper engine: exact playback, the loop point, overdub layers,
// undo/redo, switch gestures, stop modes, speed and reverse, tempo sync, and
// saving / loading / resizing the loop while "audio" is running - and while it
// starts and stops. Also the loop helpers (sample-rate conversion, overview).

#include "TestFramework.h"
#include "TestSuites.h"

// These tests compare samples for exact equality on purpose.
#if defined (__GNUC__) || defined (__clang__)
 #pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include "looper/LoopUtils.h"
#include "looper/LooperEngine.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <random>
#include <thread>
#include <vector>

using testing::check;
using testing::section;
using namespace oap::looper;

namespace
{
constexpr double kTwoPi = 6.283185307179586;

/** Drives an engine like a host would and records everything it outputs. */
struct Rig
{
    explicit Rig (double sampleRate = 48000.0) : fs (sampleRate) { engine.prepare (fs); }

    LooperEngine engine;
    Settings settings;
    Timing timing;
    double fs;
    std::int64_t now = 0;
    bool capture = true, service = true;
    std::function<float (std::int64_t)> input = [] (std::int64_t) { return 0.0f; };
    std::vector<float> out, outRight;

    void run (std::int64_t samples, int block = 256)
    {
        std::vector<float> l ((size_t) block), r ((size_t) block);
        while (samples > 0)
        {
            const int n = (int) std::min<std::int64_t> (block, samples);
            for (int i = 0; i < n; ++i)
            {
                l[(size_t) i] = input (now + i);
                r[(size_t) i] = -0.5f * l[(size_t) i]; // a different right channel
            }

            beginBlock();
            engine.process (l.data(), r.data(), n);
            if (service)
                engine.serviceMemory();

            if (capture)
            {
                out.insert (out.end(), l.begin(), l.begin() + n);
                outRight.insert (outRight.end(), r.begin(), r.begin() + n);
            }
            now += n;
            samples -= n;
        }
    }

    /** What the plugin does at the top of each block, before it passes on any switch events. */
    void beginBlock()
    {
        if (timing.hostPlaying)
        {
            timing.hostBeat = hostBeatAtStart + (double) (now - hostStartSample) * timing.bpm / 60.0 / fs;
            timing.hostBarStart = timing.beatsPerBar * std::floor (timing.hostBeat / timing.beatsPerBar);
        }
        engine.beginBlock (settings, timing);
    }

    void seconds (double s) { run ((std::int64_t) std::llround (s * fs)); }
    void tapMain()  { beginBlock(); engine.mainSwitch (true); engine.mainSwitch (false); }
    void tapStop()  { beginBlock(); engine.stopSwitch (true); engine.stopSwitch (false); }
    State state() const { return engine.getStatus().state; }
    int lengthSamples() const { return (int) std::llround (engine.getStatus().lengthSeconds * fs); }

    void startHost (double beat)
    {
        timing.hostPlaying = true;
        hostBeatAtStart = beat;
        hostStartSample = now;
    }

    double hostBeatAtStart = 0.0;
    std::int64_t hostStartSample = 0;
};

float signalA (std::int64_t n, double fs)
{
    const double t = (double) n / fs;
    return (float) (0.3 * std::sin (kTwoPi * 220.5 * t) + 0.2 * std::sin (kTwoPi * 331.7 * t + 1.0));
}

float signalB (std::int64_t n, double fs)
{
    const double t = (double) n / fs;
    return (float) (0.25 * std::sin (kTwoPi * 523.3 * t + 0.4));
}

float signalC (std::int64_t n, double fs)
{
    const double t = (double) n / fs;
    return (float) (0.2 * std::sin (kTwoPi * 147.9 * t + 2.0));
}

/** Records a loop of signal A with the given length; returns the time the loop started playing. */
std::int64_t recordLoop (Rig& rig, int lengthSamples)
{
    rig.input = [&rig] (std::int64_t n) { return signalA (n, rig.fs); };
    rig.run (9600);
    rig.tapMain();
    rig.run (lengthSamples);
    rig.tapMain();
    rig.input = [] (std::int64_t) { return 0.0f; };
    return rig.now;
}

/** One loop cycle as heard, starting at a time when the play head is at the loop start. */
std::vector<float> cycleAt (Rig& rig, std::int64_t loopStart, int length)
{
    std::int64_t t = loopStart;
    while (t < rig.now)
        t += length;
    if (rig.now < t + length)
        rig.run (t + length - rig.now);
    return { rig.out.begin() + t, rig.out.begin() + t + length };
}

double maxDifference (const std::vector<float>& a, const std::vector<float>& b, int from, int to)
{
    double d = 0.0;
    for (int i = from; i < to; ++i)
        d = std::max (d, (double) std::abs (a[(size_t) i] - b[(size_t) i]));
    return d;
}

double zeroCrossingHz (const std::vector<float>& x, std::int64_t from, std::int64_t to, double fs)
{
    int crossings = 0;
    for (std::int64_t i = from + 1; i < to; ++i)
        if ((x[(size_t) i - 1] < 0.0f) != (x[(size_t) i] < 0.0f))
            ++crossings;
    return 0.5 * crossings * fs / (double) (to - from);
}

double rms (const std::vector<float>& x, std::int64_t from, std::int64_t to)
{
    double s = 0.0;
    for (std::int64_t i = from; i < to; ++i)
        s += (double) x[(size_t) i] * x[(size_t) i];
    return std::sqrt (s / (double) std::max<std::int64_t> (1, to - from));
}

//==============================================================================
void testRecordAndPlay()
{
    section ("Looper: record and play");
    Rig rig;
    const int L = 48000;
    const auto t0 = rig.now + 9600; // when recording starts
    const auto start = recordLoop (rig, L);

    check (rig.state() == State::Playing, "second press closes the loop and plays it");
    check (rig.lengthSamples() == L, "loop length is exactly the time between the presses");

    rig.run (3 * L);
    bool exact = true;
    for (int k = 0; k < L - 600; ++k)
        exact = exact && rig.out[(size_t) (start + k)] == signalA (t0 + k, rig.fs);
    check (exact, "the loop plays back exactly what was recorded");

    bool right = true;
    for (int k = 0; k < L - 600; ++k)
        right = right && rig.outRight[(size_t) (start + k)] == -0.5f * signalA (t0 + k, rig.fs);
    check (right, "the right channel is recorded separately");

    bool repeats = true;
    for (int k = 0; k < L; ++k)
        repeats = repeats && rig.out[(size_t) (start + k)] == rig.out[(size_t) (start + L + k)];
    check (repeats, "every cycle is identical");

    // Loop point: the signal ran continuously through the start, so there must be no step at the wrap.
    double normalStep = 0.0, wrapStep = 0.0;
    for (std::int64_t i = start + 1000; i < start + 20000; ++i)
        normalStep = std::max (normalStep, (double) std::abs (rig.out[(size_t) i + 1] - rig.out[(size_t) i]));
    for (std::int64_t i = start + L - 500; i < start + L + 500; ++i)
        wrapStep = std::max (wrapStep, (double) std::abs (rig.out[(size_t) i + 1] - rig.out[(size_t) i]));
    std::printf ("  largest step in the signal %.4f, around the loop point %.4f\n", normalStep, wrapStep);
    check (wrapStep < 1.25 * normalStep, "the loop point is seamless");

    const auto st = rig.engine.getStatus();
    check (st.layers == 1 && ! st.canUndo && ! st.canRedo, "one layer, nothing to undo yet");
}

//==============================================================================
void testOverdubAndUndo()
{
    section ("Looper: overdub, undo and redo");
    Rig rig;
    const int L = 48000;
    const auto start = recordLoop (rig, L);
    const auto base = cycleAt (rig, start, L);

    // Overdub B for half a second, starting 0.25 s into the loop.
    rig.run (start + 3 * L + 12000 - rig.now);
    const auto dubStart = rig.now;
    const int dubOffset = 12000, dubLength = 24000;
    rig.input = [&rig] (std::int64_t n) { return signalB (n, rig.fs); };
    rig.tapMain();
    check (rig.state() == State::Overdubbing, "third press starts an overdub");
    rig.run (dubLength);
    rig.tapMain();
    check (rig.state() == State::Playing, "fourth press goes back to playing");
    rig.input = [] (std::int64_t) { return 0.0f; };

    const auto withB = cycleAt (rig, start, L);
    std::vector<float> expected = base;
    for (int j = 300; j < dubLength - 10; ++j)
        expected[(size_t) (dubOffset + j)] += signalB (dubStart + j, rig.fs);

    check (maxDifference (withB, expected, dubOffset + 300, dubOffset + dubLength - 10) < 1.0e-6, "the overdub is added at the position it was played");
    check (maxDifference (withB, base, 0, dubOffset - 10) == 0.0
               && maxDifference (withB, base, dubOffset + dubLength + 300, L) == 0.0,
           "the rest of the loop is untouched");

    double edge = 0.0; // the overdub fades in and out
    for (int j = -5; j < 5; ++j)
        edge = std::max (edge, (double) std::abs (withB[(size_t) (dubOffset + j)] - base[(size_t) (dubOffset + j)]));
    check (edge < 0.02, "the overdub fades in instead of clicking");

    auto st = rig.engine.getStatus();
    check (st.layers == 2 && st.canUndo && ! st.canRedo, "two layers, undo available");

    rig.engine.undoSwitch();
    const auto undone = cycleAt (rig, start, L);
    check (maxDifference (undone, base, 0, L) == 0.0, "undo restores the loop exactly");
    st = rig.engine.getStatus();
    check (st.layers == 1 && ! st.canUndo && st.canRedo, "redo available after undo");

    rig.engine.undoSwitch();
    const auto redone = cycleAt (rig, start, L);
    check (maxDifference (redone, withB, 0, L) == 0.0, "redo brings the overdub back exactly");

    // A second overdub makes the first one permanent; undo then only removes the second.
    rig.run (start + 12 * L + 30000 - rig.now);
    const auto dub2Start = rig.now;
    rig.input = [&rig] (std::int64_t n) { return signalC (n, rig.fs); };
    rig.tapMain();
    rig.run (24000); // wraps around the loop point
    rig.tapMain();
    rig.input = [] (std::int64_t) { return 0.0f; };

    const auto withBC = cycleAt (rig, start, L);
    double diff = 0.0;
    for (int j = 300; j < 24000 - 10; ++j)
    {
        const int k = (30000 + j) % L;
        diff = std::max (diff, (double) std::abs (withBC[(size_t) k] - (withB[(size_t) k] + signalC (dub2Start + j, rig.fs))));
    }
    check (diff < 1.0e-6, "a second overdub is added on top, across the loop point");
    check (rig.engine.getStatus().layers == 3, "three layers");

    rig.engine.undoSwitch();
    check (maxDifference (cycleAt (rig, start, L), withB, 0, L) == 0.0, "undo removes only the last overdub");

    // Undo, then record something new: the undone layer is gone for good.
    rig.run (start + 20 * L + 6000 - rig.now);
    const auto dub3Start = rig.now;
    rig.input = [&rig] (std::int64_t n) { return signalC (n, rig.fs); };
    rig.tapMain();
    rig.run (24000);
    rig.tapMain();
    rig.input = [] (std::int64_t) { return 0.0f; };

    const auto afterNew = cycleAt (rig, start, L);
    diff = 0.0;
    for (int j = 300; j < 24000 - 10; ++j)
        diff = std::max (diff, (double) std::abs (afterNew[(size_t) (6000 + j)] - (withB[(size_t) (6000 + j)] + signalC (dub3Start + j, rig.fs))));
    check (diff < 1.0e-6 && maxDifference (afterNew, withB, 31000, L) == 0.0,
           "overdubbing after an undo replaces the undone layer");

    rig.engine.undoSwitch();
    check (maxDifference (cycleAt (rig, start, L), withB, 0, L) == 0.0, "and can itself be undone");

    // Undo during an overdub cancels it.
    rig.engine.undoSwitch(); // redo, so there is a layer on top again
    rig.input = [&rig] (std::int64_t n) { return signalB (n, rig.fs); };
    rig.tapMain();
    rig.seconds (0.7);
    rig.engine.undoSwitch();
    rig.input = [] (std::int64_t) { return 0.0f; };
    check (rig.state() == State::Playing, "undo while overdubbing stops the overdub");
    check (maxDifference (cycleAt (rig, start, L), afterNew, 0, L) == 0.0, "and removes what it recorded");
}

//==============================================================================
void testGestures()
{
    section ("Looper: double tap, hold and the stop switch");
    Rig rig;
    const int L = 48000;
    const auto start = recordLoop (rig, L);
    const auto base = cycleAt (rig, start, L);

    rig.input = [&rig] (std::int64_t n) { return signalB (n, rig.fs); };
    rig.tapMain();
    rig.seconds (0.6);
    rig.tapMain();
    rig.input = [] (std::int64_t) { return 0.0f; };
    const auto withB = cycleAt (rig, start, L);

    // Double tap: the first tap starts an overdub, the second (soon after) takes it back and stops.
    rig.input = [&rig] (std::int64_t n) { return signalC (n, rig.fs); };
    rig.tapMain();
    rig.seconds (0.15);
    rig.tapMain();
    rig.input = [] (std::int64_t) { return 0.0f; };
    rig.seconds (0.1);
    check (rig.state() == State::Stopped, "double tap stops the loop");
    check (rms (rig.out, rig.now - 2000, rig.now) == 0.0, "nothing plays when stopped");
    check (rig.engine.getStatus().canUndo && rig.engine.getStatus().layers == 2, "the double tap didn't cost the undo of the last layer");

    rig.seconds (1.0);
    rig.tapMain();
    check (rig.state() == State::Playing, "a tap starts playback again");
    const auto playStart = rig.now;
    rig.run (2 * L);
    double diff = 0.0;
    for (int k = 200; k < L; ++k)
        diff = std::max (diff, (double) std::abs (rig.out[(size_t) (playStart + k)] - withB[(size_t) k]));
    check (diff == 0.0, "playback restarts from the top, and the double tap recorded nothing");

    // Hold: undo.
    rig.engine.mainSwitch (true);
    rig.seconds (1.0);
    rig.engine.mainSwitch (false);
    check (rig.state() == State::Playing && rig.engine.getStatus().layers == 1, "holding the switch undoes the last layer");
    check (maxDifference (cycleAt (rig, playStart, L), base, 0, L) == 0.0, "the loop is back to its first layer");

    rig.engine.mainSwitch (true);
    rig.seconds (1.0);
    rig.engine.mainSwitch (false);
    check (rig.engine.getStatus().layers == 2, "holding again redoes it");
    check (maxDifference (cycleAt (rig, playStart, L), withB, 0, L) == 0.0, "exactly");

    // Stop switch.
    rig.tapStop();
    rig.seconds (0.05);
    check (rig.state() == State::Stopped, "the stop switch stops");
    rig.engine.mainSwitch (true); // hold while stopped = clear
    rig.seconds (1.0);
    rig.engine.mainSwitch (false);
    check (rig.state() == State::Empty, "holding the main switch while stopped clears the loop");

    // Stop switch: hold = clear, also while recording.
    recordLoop (rig, L);
    rig.engine.stopSwitch (true);
    rig.seconds (1.0);
    rig.engine.stopSwitch (false);
    check (rig.state() == State::Empty, "holding the stop switch clears the loop");
    check (rig.engine.getStatus().lengthSeconds == 0.0 && rig.engine.getStatus().layers == 0, "nothing left after a clear");

    rig.tapMain();
    rig.seconds (0.5);
    rig.tapStop();
    check (rig.state() == State::Stopped && rig.lengthSamples() == 24000, "stop while recording keeps the loop, stopped");

    // A press that is too short to be a loop is ignored.
    rig.engine.clear();
    rig.seconds (0.1);
    rig.tapMain();
    rig.run (480);
    rig.tapMain();
    check (rig.state() == State::Empty, "a 10 ms recording is thrown away");
}

//==============================================================================
void testStopModesAndOrder()
{
    section ("Looper: stop modes and record -> overdub order");
    const int L = 48000;

    {
        Rig rig;
        rig.settings.stopMode = StopMode::FadeOut;
        const auto start = recordLoop (rig, L);
        rig.run (start + 2 * L - rig.now);
        rig.tapStop();
        const auto t = rig.now;
        rig.seconds (1.0);
        check (rig.state() == State::Playing && rig.engine.getStatus().stopping, "fade out: still playing after one second");
        const double early = rms (rig.out, t, t + 4800), mid = rms (rig.out, t + 48000 - 4800, t + 48000);
        rig.seconds (2.2);
        check (rig.state() == State::Stopped, "fade out: stopped after about three seconds");
        check (mid < 0.8 * early && mid > 0.3 * early, "fade out: the level falls gradually");

        rig.tapMain();
        rig.tapStop();
        rig.seconds (0.3);
        rig.tapMain(); // changes its mind
        rig.seconds (0.1);
        check (rig.state() == State::Playing && ! rig.engine.getStatus().stopping, "pressing the main switch cancels a fade out");
        rig.tapStop();
        rig.tapStop();
        rig.run (64);
        check (rig.state() == State::Stopped, "a second press on stop stops immediately");
    }

    {
        Rig rig;
        rig.settings.stopMode = StopMode::LoopEnd;
        const auto start = recordLoop (rig, L);
        rig.run (start + 2 * L + 10000 - rig.now);
        rig.tapStop();
        rig.run (30000);
        check (rig.state() == State::Playing, "loop end: keeps playing until the end of the loop");
        check (rms (rig.out, rig.now - 2000, rig.now) > 0.05, "loop end: still audible before the end");
        rig.run (8000 + 256);
        check (rig.state() == State::Stopped, "loop end: stops at the loop point");
        check (rms (rig.out, start + 3 * L, start + 3 * L + 200) == 0.0, "loop end: silent after the loop point");
    }

    {
        Rig rig;
        rig.settings.overdubAfterRecord = true;
        rig.input = [&rig] (std::int64_t n) { return signalA (n, rig.fs); };
        rig.run (9600);
        const auto t0 = rig.now;
        rig.tapMain();
        rig.run (L);
        rig.tapMain();
        const auto start = rig.now;
        check (rig.state() == State::Overdubbing, "record -> overdub: the second press goes straight to overdub");
        rig.input = [&rig] (std::int64_t n) { return signalB (n, rig.fs); };
        rig.run (30000);
        rig.tapMain();
        rig.input = [] (std::int64_t) { return 0.0f; };
        check (rig.state() == State::Playing, "record -> overdub: the third press plays");

        const auto cycle = cycleAt (rig, start, L);
        double diff = 0.0;
        for (int k = 300; k < 29990; ++k)
            diff = std::max (diff, (double) std::abs (cycle[(size_t) k] - (signalA (t0 + k, rig.fs) + signalB (start + k, rig.fs))));
        check (diff < 1.0e-6, "record -> overdub: the overdub continues seamlessly after the recording");
    }
}

//==============================================================================
void testSpeedReverseFeedback()
{
    section ("Looper: speed, reverse and overdub feedback");
    const int L = 48000;

    {
        Rig rig;
        rig.input = [&rig] (std::int64_t n) { return 0.4f * (float) std::sin (kTwoPi * 440.0 * (double) n / rig.fs); };
        rig.run (9600);
        rig.tapMain();
        rig.run (L);
        rig.tapMain();
        rig.input = [] (std::int64_t) { return 0.0f; };

        rig.run (L);
        const double normal = zeroCrossingHz (rig.out, rig.now - 40000, rig.now, rig.fs);
        rig.settings.speed = 0.5f;
        rig.run (2 * L);
        const double half = zeroCrossingHz (rig.out, rig.now - 40000, rig.now, rig.fs);
        rig.settings.speed = 2.0f;
        rig.run (2 * L);
        const double twice = zeroCrossingHz (rig.out, rig.now - 40000, rig.now, rig.fs);
        std::printf ("  440 Hz loop: 1x %.1f Hz, 1/2x %.1f Hz, 2x %.1f Hz\n", normal, half, twice);
        check (std::abs (normal - 440.0) < 2.0 && std::abs (half - 220.0) < 2.0 && std::abs (twice - 880.0) < 4.0,
               "half speed is an octave down, double speed an octave up");

        // Overdub at half speed, listen at normal speed: the overdub comes out an octave up.
        rig.settings.speed = 0.5f;
        rig.settings.feedback = 0.0f; // replace the loop, so only the new layer is heard
        rig.input = [&rig] (std::int64_t n) { return 0.4f * (float) std::sin (kTwoPi * 330.0 * (double) n / rig.fs); };
        rig.tapMain();
        rig.run (2 * L + 2000);
        rig.tapMain();
        rig.input = [] (std::int64_t) { return 0.0f; };
        rig.settings.speed = 1.0f;
        rig.settings.feedback = 1.0f;
        rig.run (2 * L);
        const double overdubbed = zeroCrossingHz (rig.out, rig.now - 40000, rig.now, rig.fs);
        std::printf ("  330 Hz overdubbed at half speed plays at %.1f Hz at normal speed\n", overdubbed);
        check (std::abs (overdubbed - 660.0) < 4.0, "an overdub made at half speed plays an octave up at normal speed");
    }

    {
        Rig rig;
        const auto start = recordLoop (rig, L);
        const auto base = cycleAt (rig, start, L);
        rig.run (start + 4 * L - rig.now);
        rig.settings.reverse = true;
        rig.run (2 * L);
        // From the loop start the reversed play head goes 0, L-1, L-2, ...
        bool reversed = true;
        const auto t = start + 4 * L;
        for (int k = 1; k < L; ++k)
            reversed = reversed && rig.out[(size_t) (t + k)] == base[(size_t) (L - k)];
        check (reversed, "reverse plays the loop backwards, sample for sample");

        rig.input = [&rig] (std::int64_t n) { return signalB (n, rig.fs); };
        rig.tapMain();
        rig.seconds (0.7);
        rig.tapMain();
        rig.input = [] (std::int64_t) { return 0.0f; };
        rig.seconds (0.1);
        rig.settings.reverse = false;
        rig.run (3 * L);
        check (rms (rig.out, rig.now - L, rig.now) > 1.02 * rms (base, 0, L), "overdubbing works in reverse");
    }

    {
        Rig rig;
        const auto start = recordLoop (rig, L);
        const auto base = cycleAt (rig, start, L);
        rig.settings.feedback = 0.5f;
        rig.run (start + 3 * L - rig.now);
        rig.tapMain(); // overdub silence for exactly one cycle
        rig.run (L);
        rig.tapMain();
        const auto faded = cycleAt (rig, start, L);
        double diff = 0.0;
        for (int k = 400; k < L - 600; ++k)
            diff = std::max (diff, (double) std::abs (faded[(size_t) k] - 0.5f * base[(size_t) k]));
        check (diff < 1.0e-6, "feedback 50 %: one overdub pass halves the old layer");
        rig.engine.undoSwitch();
        check (maxDifference (cycleAt (rig, start, L), base, 0, L) == 0.0, "and undo brings it back at full level");
    }
}

//==============================================================================
void testTempoSync()
{
    section ("Looper: tempo sync, fixed length, auto start and click");

    {
        Rig rig;
        rig.settings.sync = true;
        rig.settings.quantize = Quantize::Off;
        rig.settings.fixedBars = 2.0;
        rig.timing.bpm = 120.0;
        rig.input = [&rig] (std::int64_t n) { return signalA (n, rig.fs); };
        rig.run (4800);
        rig.tapMain();
        rig.seconds (3.9);
        check (rig.state() == State::Recording, "fixed length: still recording just before two bars");
        rig.seconds (0.2);
        check (rig.state() == State::Playing, "fixed length: recording ends by itself and the loop plays");
        check (rig.lengthSamples() == 192000, "fixed length: two bars at 120 BPM are exactly 4 seconds");
        check (std::abs (rig.engine.getStatus().loopBpm - 120.0) < 1.0e-9, "the loop remembers its tempo");

        // Tempo follow: at half the tempo the loop plays at half speed.
        rig.input = [] (std::int64_t) { return 0.0f; };
        rig.engine.clear();
        rig.seconds (0.1);
        rig.settings.fixedBars = 0.0;
        rig.settings.fixedBeats = 2.0;
        rig.input = [&rig] (std::int64_t n) { return 0.4f * (float) std::sin (kTwoPi * 440.0 * (double) n / rig.fs); };
        rig.tapMain();
        rig.seconds (1.1);
        rig.input = [] (std::int64_t) { return 0.0f; };
        check (rig.state() == State::Playing && rig.lengthSamples() == 48000, "fixed length: two beats at 120 BPM are one second");
        rig.timing.bpm = 60.0;
        rig.seconds (3.0);
        const double hz = zeroCrossingHz (rig.out, rig.now - 48000, rig.now, rig.fs);
        check (std::abs (hz - 220.0) < 2.0, "the loop follows the tempo: half tempo plays at half speed");
    }

    {
        Rig rig;
        rig.settings.sync = true;
        rig.settings.quantize = Quantize::Bar;
        rig.settings.overdubAfterRecord = true;
        rig.settings.fixedBars = 1.0;
        rig.timing.bpm = 100.0;
        constexpr std::int64_t barLine = 72000, loopEnd = barLine + 115200;
        rig.input = [&rig] (std::int64_t n) { return n < loopEnd ? signalA (n, rig.fs) : 0.0f; };
        rig.startHost (1.5); // the host is playing, half way through beat 2
        rig.seconds (0.2);
        rig.tapMain();
        check (rig.state() == State::Armed, "quantise: the press arms recording");
        // The next bar line is at beat 4: (4 - 1.5) beats * 0.6 s = 1.5 s after the host started.
        rig.run ((std::int64_t) (1.5 * rig.fs) - 9600 - 512);
        check (rig.state() == State::Armed, "quantise: still waiting just before the bar line");
        rig.run (1024);
        check (rig.state() == State::Recording, "quantise: recording starts on the bar line");
        rig.seconds (2.5);
        check (rig.state() == State::Overdubbing, "one bar later the loop closes and overdubbing continues");
        check (std::abs (rig.lengthSamples() - 115200) <= 1, "the loop is exactly one bar long (2.4 s at 100 BPM)");
        rig.tapMain();

        // The loop started on the bar line (1.5 s in): its first sample is the input at that moment.
        bool aligned = true;
        for (int k = 0; k < 2000; ++k)
            aligned = aligned && rig.out[(size_t) (loopEnd + k)] == signalA (barLine + k, rig.fs);
        check (aligned, "the recording starts on the exact sample of the bar line");
    }

    {
        Rig rig;
        rig.settings.sync = true;
        rig.settings.quantize = Quantize::Beat;
        rig.timing.bpm = 120.0;
        rig.startHost (0.25);
        rig.input = [&rig] (std::int64_t n) { return signalA (n, rig.fs); };
        rig.seconds (0.1);
        rig.tapMain();
        rig.seconds (1.3);
        check (rig.state() == State::Recording, "quantise to beats: recording");
        rig.tapMain();
        check (rig.state() == State::Recording, "the closing press waits for the next beat too");
        rig.seconds (0.6);
        check (rig.state() == State::Playing, "and then closes the loop");
        const int len = rig.lengthSamples();
        check (len % 24000 == 0 && len > 0, "a quantised free-length loop is a whole number of beats long");
    }

    {
        Rig rig;
        rig.settings.autoStart = true;
        rig.run (256);
        rig.tapMain();
        rig.seconds (0.5);
        check (rig.state() == State::Armed, "auto start: waits while the input is silent");
        const auto noteAt = rig.now + 1000;
        rig.input = [&rig, noteAt] (std::int64_t n) { return n >= noteAt ? 0.5f * (float) std::sin (kTwoPi * 220.0 * (double) (n - noteAt) / rig.fs) : 0.0f; };
        rig.seconds (0.5);
        check (rig.state() == State::Recording, "auto start: the first note starts the recording");
        rig.tapMain();
        const auto start = rig.now;
        rig.input = [] (std::int64_t) { return 0.0f; };
        rig.seconds (1.0);
        // The note began 5 ms into the loop at most (the pre-roll), from silence.
        int firstLoud = 0;
        while (firstLoud < 2000 && std::abs (rig.out[(size_t) (start + firstLoud)]) < 0.01f)
            ++firstLoud;
        check (firstLoud > 50 && firstLoud < 300, "auto start: the loop begins a few milliseconds before the note, so its attack is complete");

        rig.engine.clear();
        rig.seconds (0.1);
        rig.tapMain();
        rig.tapMain();
        check (rig.state() == State::Empty, "pressing again while armed cancels");
    }

    {
        Rig rig;
        rig.settings.click = true;
        rig.timing.bpm = 120.0;
        rig.seconds (2.0);
        check (rms (rig.out, 0, rig.now) > 0.005, "the click is audible");
        int clicks = 0;
        bool inClick = false;
        for (std::int64_t i = 0; i < rig.now; ++i)
        {
            const bool loud = std::abs (rig.out[(size_t) i]) > 0.05f;
            if (loud && ! inClick)
                ++clicks;
            if (loud)
                inClick = true;
            else if (i % 4800 == 4799)
                inClick = false;
        }
        check (clicks == 4, "four beats in two seconds at 120 BPM");

        rig.tapMain();
        rig.seconds (1.0);
        rig.tapMain();
        rig.settings.click = false;
        rig.seconds (1.2);
        check (rms (rig.out, rig.now - 48000, rig.now) < 1.0e-6, "the click is not recorded into the loop");
    }
}

//==============================================================================
void testMemoryAndLimits()
{
    section ("Looper: memory and limits");

    {
        Rig rig (8000.0);
        rig.capture = false;
        rig.input = [] (std::int64_t n) { return 0.1f * (float) ((n % 100) - 50) / 50.0f; };
        rig.tapMain();
        rig.run ((std::int64_t) (8000.0 * 301.0), 2048);
        check (rig.state() == State::Playing, "recording stops by itself at the maximum length");
        check (rig.lengthSamples() == (int) (LooperEngine::maxSeconds * 8000.0), "the maximum loop length is five minutes");
        rig.engine.clear();
        rig.seconds (0.2);
        check (rig.state() == State::Empty, "and a five-minute loop can be cleared");
    }

    {
        Rig rig (8000.0);
        rig.capture = false;
        rig.service = false; // nobody hands over more memory
        rig.input = [] (std::int64_t n) { return 0.1f * (float) ((n % 100) - 50) / 50.0f; };
        rig.tapMain();
        rig.run (8000 * 60, 512);
        check (rig.state() == State::Playing, "if memory runs out, the loop simply ends there");
        check (rig.lengthSamples() >= 80000 && rig.lengthSamples() < 8000 * 40, "with the memory that was kept ready");
    }

    {
        Rig rig;
        const int L = 48000;
        const auto start = recordLoop (rig, L);
        const auto base = cycleAt (rig, start, L);
        rig.engine.prepare (48000.0);
        check (rig.state() == State::Playing && rig.lengthSamples() == L, "preparing again at the same sample rate keeps the loop");
        rig.engine.prepare (44100.0);
        check (rig.engine.getStatus().state == State::Empty, "a different sample rate starts with an empty looper");
    }
}

//==============================================================================
void testCopyLoadResize()
{
    section ("Looper: copy, load, double and halve");
    Rig rig;
    const int L = 48000;
    const auto start = recordLoop (rig, L);
    const auto base = cycleAt (rig, start, L);

    rig.input = [&rig] (std::int64_t n) { return signalB (n, rig.fs); };
    rig.tapMain();
    rig.seconds (0.7);
    rig.tapMain();
    rig.input = [] (std::int64_t) { return 0.0f; };
    const auto withB = cycleAt (rig, start, L);

    std::vector<float> l, r;
    double bpm = 0.0;
    std::uint32_t generation = 0;
    check (rig.engine.copyLoop (l, r, bpm, generation, true) == LooperEngine::Result::ok && (int) l.size() == L, "copy returns the whole loop");
    check (maxDifference (l, withB, 0, L) == 0.0, "the copy is what the loop sounds like");
    bool rightOk = true;
    for (int k = 0; k < L - 600; k += 7)
        rightOk = rightOk && std::abs (r[(size_t) k] + 0.5f * l[(size_t) k]) < 1.0e-6f;
    check (rightOk, "both channels are copied");
    check (generation == rig.engine.getGeneration(), "the copy is tagged with the loop's generation");

    rig.engine.undoSwitch();
    rig.run (256);
    check (rig.engine.copyLoop (l, r, bpm, generation, true) == LooperEngine::Result::ok && maxDifference (l, base, 0, L) == 0.0,
           "after an undo the copy is the loop without the undone layer");
    rig.engine.undoSwitch();
    rig.run (256);

    // Double: the loop repeats, then an overdub can span both halves; undo and halve get back.
    check (rig.engine.doubleLength (true) == LooperEngine::Result::ok, "double length succeeds");
    rig.run (256);
    check (rig.lengthSamples() == 2 * L, "the loop is twice as long");
    check (rig.engine.copyLoop (l, r, bpm, generation, true) == LooperEngine::Result::ok && (int) l.size() == 2 * L, "copy of the doubled loop");
    bool repeated = true;
    for (int k = 0; k < L; ++k)
        repeated = repeated && l[(size_t) k] == withB[(size_t) k] && l[(size_t) (L + k)] == withB[(size_t) k];
    check (repeated, "both halves are the old loop");
    check (! rig.engine.getStatus().canUndo, "doubling makes the layers permanent");

    rig.input = [&rig] (std::int64_t n) { return signalC (n, rig.fs); };
    rig.tapMain();
    rig.seconds (1.5);
    rig.tapMain();
    rig.input = [] (std::int64_t) { return 0.0f; };
    rig.seconds (0.2);
    check (rig.engine.getStatus().canUndo && rig.engine.getStatus().layers == 3, "overdubbing the doubled loop");
    rig.engine.undoSwitch();
    rig.run (256);

    check (rig.engine.halveLength (true) == LooperEngine::Result::ok, "halve length succeeds");
    rig.run (256);
    check (rig.lengthSamples() == L, "the loop is back to its length");
    check (rig.engine.copyLoop (l, r, bpm, generation, true) == LooperEngine::Result::ok && maxDifference (l, withB, 0, L - 600) == 0.0,
           "and to its content");

    // Load replaces the loop and leaves it ready to play.
    std::vector<float> ramp ((size_t) 30000), ramp2 ((size_t) 30000);
    for (size_t i = 0; i < ramp.size(); ++i)
    {
        ramp[i] = (float) std::sin (0.01 * (double) i);
        ramp2[i] = 0.25f * ramp[i];
    }
    check (rig.engine.loadLoop (ramp.data(), ramp2.data(), 30000, 97.0, true) == LooperEngine::Result::ok, "load succeeds");
    rig.run (256);
    auto st = rig.engine.getStatus();
    check (st.state == State::Stopped && rig.lengthSamples() == 30000 && st.layers == 1 && std::abs (st.loopBpm - 97.0) < 1.0e-9,
           "the loaded loop is stopped and ready, with its tempo");
    rig.tapMain();
    const auto playStart = rig.now;
    rig.run (60000);
    bool loaded = true;
    for (int k = 200; k < 30000; ++k)
        loaded = loaded && rig.out[(size_t) (playStart + k)] == ramp[(size_t) k] && rig.outRight[(size_t) (playStart + k)] == ramp2[(size_t) k];
    check (loaded, "the loaded loop plays back exactly");

    check (rig.engine.loadLoop (nullptr, nullptr, 0, 0.0, true) == LooperEngine::Result::ok, "loading nothing");
    rig.run (256);
    check (rig.state() == State::Empty, "clears the looper");
    check (rig.engine.copyLoop (l, r, bpm, generation, true) == LooperEngine::Result::empty && l.empty(), "copying an empty looper says so");
    check (rig.engine.doubleLength (true) == LooperEngine::Result::empty && rig.engine.halveLength (true) == LooperEngine::Result::empty,
           "nothing to resize when empty");
}

//==============================================================================
void testWithAudioThread()
{
    section ("Looper: other threads while audio is running");
    LooperEngine engine;
    engine.prepare (48000.0);
    std::atomic<bool> quit { false };
    std::atomic<int> blocks { 0 };
    std::atomic<bool> finite { true };

    std::thread audio ([&]
    {
        std::mt19937 rng (5);
        std::uniform_real_distribution<float> noise (-0.2f, 0.2f);
        std::vector<float> l (256), r (256);
        Settings settings;
        Timing timing;
        int counter = 0;

        while (! quit.load())
        {
            for (size_t i = 0; i < l.size(); ++i)
                l[i] = r[i] = noise (rng);

            engine.beginBlock (settings, timing);

            // record 0.5 s, play, overdub now and then, undo, clear every so often
            const int phase = counter % 1500;
            const bool pressMain = phase == 10 || phase == 104 || phase == 400 || phase == 560;
            const bool pressUndo = phase == 700 || phase == 800;
            const bool pressClear = phase == 1400;
            bool wait = false;

            if (pressMain || pressUndo || pressClear)
            {
                if (! engine.acceptsEvents())
                {
                    engine.requestAbort(); // another thread has the loop: ask it to hurry, try again next block
                    wait = true;
                }
                else if (pressMain)
                {
                    engine.mainSwitch (true);
                    engine.mainSwitch (false);
                }
                else if (pressUndo)
                {
                    engine.undoSwitch();
                }
                else
                {
                    engine.clear();
                }
            }

            engine.process (l.data(), r.data(), 256);
            for (float v : l)
                if (! std::isfinite (v))
                    finite.store (false);
            if (! wait)
                ++counter;
            blocks.fetch_add (1);
            std::this_thread::sleep_for (std::chrono::microseconds (150));
        }
    });

    int copies = 0, busy = 0, resized = 0, loads = 0;
    bool consistent = true;
    std::vector<float> l, r, tone (20000, 0.1f);
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds (2500);

    while (std::chrono::steady_clock::now() < until)
    {
        engine.serviceMemory();
        double bpm = 0.0;
        std::uint32_t generation = 0;
        const auto result = engine.copyLoop (l, r, bpm, generation);
        if (result == LooperEngine::Result::ok)
        {
            ++copies;
            consistent = consistent && l.size() == r.size() && ! l.empty();
            for (size_t i = 0; i < l.size(); i += 97)
                consistent = consistent && std::isfinite (l[i]) && std::abs (l[i]) < 10.0f;
        }
        else if (result == LooperEngine::Result::busy)
        {
            ++busy;
        }

        if (engine.doubleLength() == LooperEngine::Result::ok)
            ++resized;
        if (engine.halveLength() == LooperEngine::Result::ok)
            ++resized;
        if (copies % 7 == 3 && engine.loadLoop (tone.data(), tone.data(), (int) tone.size(), 120.0) == LooperEngine::Result::ok)
            ++loads;

        std::this_thread::sleep_for (std::chrono::milliseconds (3));
    }

    quit.store (true);
    audio.join();
    std::printf ("  %d audio blocks; %d copies, %d busy, %d resizes, %d loads from the other thread\n",
                 blocks.load(), copies, busy, resized, loads);
    check (finite.load(), "audio output stays finite");
    check (consistent, "every copy is a complete, sane loop");
    check (copies > 5 && resized > 2 && loads > 0, "copies, resizes and loads all happen while audio runs");
    check (blocks.load() > 500, "the audio thread is never blocked");
}

//==============================================================================
void testStress()
{
    section ("Looper: random stress");
    std::mt19937 rng (99);
    std::uniform_real_distribution<float> uni (0.0f, 1.0f);
    bool finite = true, bounded = true, sane = true;

    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        Rig rig (fs);
        rig.capture = false;
        rig.input = [&rng] (std::int64_t) { return 0.1f * (std::uniform_real_distribution<float> (-1.0f, 1.0f)) (rng); };
        std::vector<float> l (2048), r (2048);
        bool mainDown = false, stopDown = false;

        for (int step = 0; step < 6000; ++step)
        {
            const float pick = uni (rng);
            if (pick < 0.10f)       { mainDown = ! mainDown; rig.engine.mainSwitch (mainDown); }
            else if (pick < 0.14f)  { stopDown = ! stopDown; rig.engine.stopSwitch (stopDown); }
            else if (pick < 0.17f)  rig.engine.undoSwitch();
            else if (pick < 0.175f) rig.engine.clear();
            else if (pick < 0.20f)  rig.settings.speed = uni (rng) < 0.33f ? 0.5f : (uni (rng) < 0.5f ? 1.0f : 2.0f);
            else if (pick < 0.22f)  rig.settings.reverse = ! rig.settings.reverse;
            else if (pick < 0.24f)  rig.settings.feedback = uni (rng);
            else if (pick < 0.26f)  rig.settings.sync = ! rig.settings.sync;
            else if (pick < 0.28f)  rig.settings.quantize = (Quantize) (int) (uni (rng) * 2.99f);
            else if (pick < 0.30f)  { rig.settings.fixedBars = uni (rng) < 0.5f ? 0.0 : 1.0; rig.settings.fixedBeats = uni (rng) < 0.5f ? 0.0 : 2.0; }
            else if (pick < 0.32f)  rig.settings.autoStart = ! rig.settings.autoStart;
            else if (pick < 0.34f)  rig.settings.click = ! rig.settings.click;
            else if (pick < 0.36f)  rig.settings.overdubAfterRecord = ! rig.settings.overdubAfterRecord;
            else if (pick < 0.38f)  rig.settings.stopMode = (StopMode) (int) (uni (rng) * 2.99f);
            else if (pick < 0.40f)  rig.timing.bpm = 60.0 + 180.0 * uni (rng);
            else if (pick < 0.41f)  { if (rig.timing.hostPlaying) rig.timing.hostPlaying = false; else rig.startHost (uni (rng) * 64.0); }
            else if (pick < 0.415f) rig.engine.doubleLength (true);
            else if (pick < 0.42f)  rig.engine.halveLength (true);
            else if (pick < 0.425f) { std::vector<float> a, b; double bpm; std::uint32_t g; rig.engine.copyLoop (a, b, bpm, g, true); }

            const int n = 1 + (int) (uni (rng) * uni (rng) * 2047.0f);
            for (int i = 0; i < n; ++i)
                l[(size_t) i] = r[(size_t) i] = rig.input (0);

            rig.beginBlock();
            rig.engine.process (l.data(), r.data(), n);
            rig.engine.serviceMemory();
            rig.now += n;

            for (int i = 0; i < n; ++i)
            {
                finite = finite && std::isfinite (l[(size_t) i]) && std::isfinite (r[(size_t) i]);
                bounded = bounded && std::abs (l[(size_t) i]) < 50.0f;
            }

            const auto st = rig.engine.getStatus();
            sane = sane && st.layers >= 0 && st.lengthSeconds >= 0.0 && st.lengthSeconds <= LooperEngine::maxSeconds + 1.0
                        && st.positionSeconds >= 0.0 && (st.lengthSeconds == 0.0 || st.positionSeconds <= st.lengthSeconds + 1.0);
        }
    }

    check (finite, "output stays finite with random presses, settings, tempo changes and block sizes");
    check (bounded, "output stays bounded");
    check (sane, "the reported status is always sane");
}

//==============================================================================
void testLoopIdentity()
{
    section ("Looper: loop identity");
    Rig rig;
    const auto empty = rig.engine.getLoopId();

    recordLoop (rig, 48000);
    const auto first = rig.engine.getLoopId();
    check (first != empty, "a new recording is a new loop");

    rig.input = [&rig] (std::int64_t n) { return signalB (n, rig.fs); };
    rig.tapMain();
    rig.seconds (0.7);
    rig.tapMain();
    rig.input = [] (std::int64_t) { return 0.0f; };
    rig.seconds (0.6);
    rig.engine.undoSwitch();
    rig.run (256);
    rig.engine.undoSwitch();
    rig.run (256);
    check (rig.engine.doubleLength (true) == LooperEngine::Result::ok, "(double)");
    rig.run (256);
    check (rig.engine.halveLength (true) == LooperEngine::Result::ok, "(halve)");
    rig.run (256);
    check (rig.engine.getLoopId() == first, "overdub, undo, redo, double and halve keep the loop's identity");

    std::vector<float> tone (20000, 0.1f);
    check (rig.engine.loadLoop (tone.data(), tone.data(), 20000, 0.0, true) == LooperEngine::Result::ok, "(load)");
    rig.run (256);
    const auto loaded = rig.engine.getLoopId();
    check (loaded != first, "a loaded loop is a new loop");

    rig.engine.clear();
    rig.run (256);
    const auto cleared = rig.engine.getLoopId();
    check (cleared != loaded && rig.state() == State::Empty, "clearing ends the loop's identity");

    recordLoop (rig, 30000);
    check (rig.engine.getLoopId() != cleared, "and the next recording gets its own");
}

//==============================================================================
void testAudioStartingAndStopping()
{
    // When no block has started for a second the engine decides that audio isn't running and
    // lets the other thread work on the loop directly. Audio may start again at any moment.
    section ("Looper: audio starts and stops while another thread uses the loop");
    LooperEngine engine;
    engine.prepare (48000.0);

    std::vector<float> tone ((size_t) 480000);
    for (size_t i = 0; i < tone.size(); ++i)
        tone[i] = (float) (0.2 * std::sin (kTwoPi * 440.0 * (double) i / 48000.0));
    check (engine.loadLoop (tone.data(), tone.data(), (int) tone.size(), 0.0, true) == LooperEngine::Result::ok, "(load)");

    std::atomic<bool> quit { false }, finite { true };
    std::atomic<int> blocks { 0 }, bursts { 0 };

    std::thread audio ([&]
    {
        Settings settings;
        Timing timing;
        std::vector<float> l (256), r (256);

        while (! quit.load())
        {
            ++bursts;
            const auto burstEnd = std::chrono::steady_clock::now() + std::chrono::milliseconds (250);

            while (! quit.load() && std::chrono::steady_clock::now() < burstEnd)
            {
                std::fill (l.begin(), l.end(), 0.05f);
                std::fill (r.begin(), r.end(), 0.05f);
                engine.beginBlock (settings, timing);
                if (engine.acceptsEvents() && engine.getStatus().state == State::Stopped)
                {
                    engine.mainSwitch (true); // play
                    engine.mainSwitch (false);
                }
                engine.process (l.data(), r.data(), 256);
                for (float v : l)
                    if (! std::isfinite (v))
                        finite.store (false);
                ++blocks;
                std::this_thread::sleep_for (std::chrono::microseconds (200));
            }

            // ...and now audio stops for longer than the engine waits for it
            for (int i = 0; i < 115 && ! quit.load(); ++i)
                std::this_thread::sleep_for (std::chrono::milliseconds (10));
        }
    });

    int copies = 0, loads = 0, resized = 0;
    bool consistent = true;
    std::vector<float> l, r;
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds (4300);

    while (std::chrono::steady_clock::now() < until)
    {
        engine.serviceMemory();
        double bpm = 0.0;
        std::uint32_t generation = 0;

        if (engine.copyLoop (l, r, bpm, generation) == LooperEngine::Result::ok)
        {
            ++copies;
            consistent = consistent && l.size() == r.size() && ! l.empty();
            for (size_t i = 0; i < l.size(); i += 997)
                consistent = consistent && std::isfinite (l[i]) && std::abs (l[i]) < 1.0f;
        }

        if (copies % 5 == 2 && engine.loadLoop (tone.data(), tone.data(), (int) tone.size(), 0.0) == LooperEngine::Result::ok)
            ++loads;
        if (copies % 9 == 4 && engine.halveLength() == LooperEngine::Result::ok)
            ++resized;
        if (copies % 9 == 7 && engine.doubleLength() == LooperEngine::Result::ok)
            ++resized;
    }

    quit.store (true);
    audio.join();
    std::printf ("  %d audio blocks in %d bursts; %d copies, %d loads, %d resizes from the other thread\n",
                 blocks.load(), bursts.load(), copies, loads, resized);
    check (finite.load(), "audio output stays finite");
    check (consistent, "every copy is a complete, sane loop");
    check (bursts.load() >= 3 && copies > 20 && loads > 2, "the loop is copied and replaced with audio running and stopped");
}

//==============================================================================
void testLoopUtils()
{
    section ("Looper: sample-rate conversion and overview");

    // A loop holding a whole number of cycles is still exactly that loop at another rate:
    // same pitch, nothing shifted, and seamless across the loop point.
    const auto tone = [] (int samples, int cycles)
    {
        std::vector<float> v ((size_t) samples);
        for (int i = 0; i < samples; ++i)
            v[(size_t) i] = (float) (0.5 * std::sin (kTwoPi * (double) cycles * (double) i / (double) samples + 0.3));
        return v;
    };

    const auto convert = [] (const std::vector<float>& in, int outSamples)
    {
        std::vector<float> out ((size_t) outSamples);
        resampleLoop (in.data(), (int) in.size(), out.data(), outSamples);
        return out;
    };

    const auto worst = [] (const std::vector<float>& a, const std::vector<float>& b)
    {
        double e = 0.0;
        for (size_t i = 0; i < a.size() && i < b.size(); ++i)
            e = std::max (e, (double) std::abs (a[i] - b[i]));
        return e;
    };

    check (resampledLength (48000, 48000.0, 44100.0, 1 << 30) == 44100 && resampledLength (44100, 44100.0, 96000.0, 1 << 30) == 96000,
           "the converted loop has the same duration");
    check (resampledLength (48000, 48000.0, 96000.0, 50000) == 50000 && resampledLength (0, 48000.0, 96000.0, 50000) == 0,
           "the length is limited, and nothing stays nothing");

    const double e1 = worst (convert (tone (48000, 1000), 44100), tone (44100, 1000));
    const double e2 = worst (convert (tone (48000, 15000), 44100), tone (44100, 15000));
    const double e3 = worst (convert (tone (44100, 5000), 96000), tone (96000, 5000));
    std::printf ("  worst error: 1 kHz down %.1f dB, 15 kHz down %.1f dB, 5 kHz up %.1f dB\n",
                 20.0 * std::log10 (e1 + 1.0e-12), 20.0 * std::log10 (e2 + 1.0e-12), 20.0 * std::log10 (e3 + 1.0e-12));
    check (e1 < 1.0e-3 && e2 < 1.0e-3 && e3 < 1.0e-3, "tones come out exact, including the samples around the loop point");

    double alias = 0.0;
    for (float v : convert (tone (48000, 23500), 44100))
        alias = std::max (alias, (double) std::abs (v));
    std::printf ("  23.5 kHz converted to 44.1 kHz: %.1f dB\n", 20.0 * std::log10 (alias / 0.5 + 1.0e-12));
    check (alias < 0.005, "what the new rate can't hold is filtered out instead of aliasing");

    const auto same = convert (tone (48000, 440), 48000);
    check (worst (same, tone (48000, 440)) == 0.0, "the same length is a plain copy");

    std::vector<float> left = tone (48000, 300), right = tone (48000, 700);
    resampleLoop (left, right, 48000.0, 44100.0, 1 << 30);
    check (left.size() == 44100 && right.size() == 44100 && worst (left, tone (44100, 300)) < 1.0e-3
               && worst (right, tone (44100, 700)) < 1.0e-3,
           "both channels of a stereo loop are converted");

    // overview
    std::vector<float> ramp ((size_t) 10000), peaks;
    for (size_t i = 0; i < ramp.size(); ++i)
        ramp[i] = (float) i / 10000.0f * ((i & 1) != 0 ? -1.0f : 1.0f);
    makeOverview (ramp.data(), ramp.data(), (int) ramp.size(), 100, peaks);
    bool rising = peaks.size() == 100;
    for (size_t i = 1; i < peaks.size(); ++i)
        rising = rising && peaks[i] > peaks[i - 1];
    check (rising && std::abs (peaks.back() - 0.9999f) < 1.0e-4f, "the overview holds the peak of every slice");
    makeOverview (ramp.data(), nullptr, 3, 100, peaks);
    check (peaks.size() == 100, "more slices than samples is fine");
}

} // namespace

//==============================================================================
void runLooperTests()
{
    testRecordAndPlay();
    testOverdubAndUndo();
    testGestures();
    testStopModesAndOrder();
    testSpeedReverseFeedback();
    testTempoSync();
    testMemoryAndLimits();
    testCopyLoadResize();
    testLoopIdentity();
    testWithAudioThread();
    testAudioStartingAndStopping();
    testStress();
    testLoopUtils();
}
