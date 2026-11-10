// Tests for the effect amps: model definitions, stability, levels, CPU, and
// that every FX control on every amp actually changes the sound.

#include "TestFramework.h"
#include "TestSignals.h"
#include "TestSuites.h"

#include "amps/EffectAmpEngine.h"

#include <chrono>
#include <random>
#include <set>

using testing::check;
using testing::section;
using namespace oap::amps;
using namespace testsignals;

namespace
{
constexpr double kTwoPi = 6.283185307179586;

void randomise (const AmpModel& m, EffectAmpEngine::Params& p, std::mt19937& rng)
{
    std::uniform_real_distribution<float> k (0.0f, 10.0f), u (0.0f, 1.0f);
    p.gain = k (rng); p.bass = k (rng); p.middle = k (rng); p.treble = k (rng); p.master = k (rng);
    p.outputDb = k (rng) * 2.4f - 12.0f;
    p.gateDb = -90.0f + 5.0f * k (rng);
    p.cabinet = (int) (k (rng) / 2.0f); // 0..4 (4 = user IR, falls back to the default cab)
    for (size_t i = 0; i < m.fxControls.size(); ++i)
        p.fx[i] = m.fxControls[i].fromNormalised (u (rng));
}

juce::AudioBuffer<float> render (const AmpModel& m, const juce::AudioBuffer<float>& input, double fs,
                                 const EffectAmpEngine::Params& p, int block = 256, double extraSeconds = 0.0)
{
    EffectAmpEngine engine (m);
    engine.prepare (fs, block);
    const int n = input.getNumSamples() + (int) (extraSeconds * fs);
    juce::AudioBuffer<float> io (2, n);
    io.clear();
    io.copyFrom (0, 0, input, 0, 0, input.getNumSamples());

    for (int start = 0; start < n; start += block)
    {
        juce::AudioBuffer<float> view (io.getArrayOfWritePointers(), 2, start, juce::jmin (block, n - start));
        engine.process (view, 1, p);
    }
    return io;
}

double stereoRmsDb (const juce::AudioBuffer<float>& b, int start = 0, int length = -1)
{
    if (length < 0)
        length = b.getNumSamples() - start;
    const double l = std::pow (10.0, rmsDb (b.getReadPointer (0, start), length) / 10.0);
    const double r = std::pow (10.0, rmsDb (b.getReadPointer (1, start), length) / 10.0);
    return 10.0 * std::log10 ((l + r) / 2.0 + 1.0e-30);
}

//==============================================================================
void testModels()
{
    section ("Effect amp models");
    std::set<std::string> ids;

    for (const auto& m : allAmpModels())
    {
        check (ids.insert (m.id).second, m.id + ": unique id");
        check (! m.voicing.stages.empty() && m.voicing.stages.size() <= 4, m.id + ": 1-4 preamp stages");
        check (m.voicing.oversamplingOrder >= 2 && m.voicing.oversamplingOrder <= 3, m.id + ": 4x or 8x oversampling");
        check (m.fxControls.size() >= 2 && m.fxControls.size() <= (size_t) EffectAmpEngine::maxFxControls,
               m.id + ": 2-5 FX controls");
        check (! m.effects.empty(), m.id + ": has built-in effects");
        check ((bool) m.applyControls, m.id + ": has an FX control mapping");

        std::set<std::string> controlIds;
        for (const auto& c : m.fxControls)
        {
            check (controlIds.insert (c.id).second, m.id + ": unique FX control id " + c.id);
            check (c.def >= c.min && c.def <= c.max, m.id + ": " + c.id + " default inside its range");
        }

        // Every effect exists, every fixed value and every mapped parameter names a real parameter.
        std::vector<std::unique_ptr<oap::fx::Effect>> owned;
        std::vector<oap::fx::Effect*> effects;
        int unknown = 0;
        for (const auto& def : m.effects)
        {
            auto e = oap::fx::createEffect (def.effectId);
            check (e != nullptr, m.id + ": effect '" + def.effectId + "' exists");
            effects.push_back (e.get());
            owned.push_back (std::move (e));
        }

        EffectSetter setter (effects, &unknown);
        for (size_t i = 0; i < m.effects.size(); ++i)
            for (const auto& [id, value] : m.effects[i].fixed)
                setter.set (i, id.c_str(), value);

        float values[EffectAmpEngine::maxFxControls] {};
        for (float t : { 0.0f, 0.5f, 1.0f })
        {
            for (size_t i = 0; i < m.fxControls.size(); ++i)
                values[i] = m.fxControls[i].fromNormalised (t);
            if (m.applyControls)
                m.applyControls (values, setter);
        }
        check (unknown == 0, m.id + ": all effect/parameter names in the model are valid (" + std::to_string (unknown) + " unknown)");
    }

    check (allAmpModels().size() == 21, "21 effect amps");
    check (findAmpModel ("dream") != nullptr && findAmpModel ("nope") == nullptr, "findAmpModel");
}

//==============================================================================
void testStress()
{
    section ("Effect amp stress test (random parameters, 44.1-96 kHz, odd and oversized blocks)");
    std::mt19937 rng (11);
    std::uniform_real_distribution<float> noise (-1.0f, 1.0f);

    for (const auto& m : allAmpModels())
    {
        bool finite = true;
        float peak = 0.0f;

        for (double fs : { 44100.0, 48000.0, 96000.0 })
            for (int block : { 1, 37, 480, 2048 })
            {
                EffectAmpEngine engine (m);
                engine.prepare (fs, block == 2048 ? 512 : block); // hosts may exceed the prepared size
                const int total = (int) (fs * (block == 1 ? 0.08 : 0.7));
                juce::AudioBuffer<float> buf (2, total);

                for (int i = 0; i < total; ++i)
                {
                    const double t = i / fs;
                    float x = 0.0f;
                    if (t < 0.25)      x = 0.9f * (float) std::sin (kTwoPi * (50.0 + 8000.0 * t) * t);
                    else if (t < 0.45) x = noise (rng);
                    else if (t < 0.5)  x = (i % 3000 == 0) ? 1.0f : 0.0f;
                    buf.setSample (0, i, x);
                    buf.setSample (1, i, -0.5f * x);
                }

                auto params = engine.defaultParams();
                for (int start = 0, b = 0; start < total; start += block, ++b)
                {
                    const int len = juce::jmin (block, total - start);
                    if (b % 3 == 0)
                        randomise (m, params, rng);

                    juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, start, len);
                    engine.process (view, 2, params);

                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < len; ++i)
                        {
                            const float v = view.getSample (ch, i);
                            finite = finite && std::isfinite (v);
                            peak = std::max (peak, std::abs (v));
                        }
                }
            }

        check (finite, m.id + ": output stays finite");
        check (peak < 24.0f, m.id + ": output bounded (peak " + std::to_string (testing::db (peak)) + " dBFS)");
    }
}

//==============================================================================
void testSilence()
{
    section ("Effect amps decay to silence (defaults, gate off)");
    const double fs = 48000.0;

    for (const auto& m : allAmpModels())
    {
        EffectAmpEngine probe (m);
        const double tail = juce::jmin (25.0, probe.getTailSeconds() + 1.5);

        juce::AudioBuffer<float> note (1, (int) fs);
        for (int i = 0; i < note.getNumSamples(); ++i)
            note.setSample (0, i, 0.4f * (float) std::sin (kTwoPi * 110.0 * i / fs));

        auto params = probe.defaultParams();
        params.gateDb = -90.0f;
        auto out = render (m, note, fs, params, 256, tail);
        const int last = (int) (0.5 * fs);
        const double level = stereoRmsDb (out, out.getNumSamples() - last, last);
        check (level < -90.0, m.id + ": silent after its tail (" + std::to_string ((int) level) + " dBFS)");
    }
}

//==============================================================================
void testLevelsAndCpu()
{
    section ("Effect amp levels at default settings, CPU and latency");
    const double fs = 48000.0;
    const auto riff = heavyRiff (fs);
    const auto arp = cleanArpeggio (fs);

    std::printf ("  %-12s %16s %16s %8s %8s %7s\n", "amp", "riff pk/rms", "arpeggio pk/rms", "latency", "cpu %", "tail s");

    for (const auto& m : allAmpModels())
    {
        EffectAmpEngine probe (m);
        const auto params = probe.defaultParams();

        const auto t0 = std::chrono::steady_clock::now();
        auto r = render (m, riff, fs, params);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        auto a = render (m, arp, fs, params);

        const double rPeak = std::max (peakDb (r.getReadPointer (0), r.getNumSamples()), peakDb (r.getReadPointer (1), r.getNumSamples()));
        const double aPeak = std::max (peakDb (a.getReadPointer (0), a.getNumSamples()), peakDb (a.getReadPointer (1), a.getNumSamples()));
        const double rRms = stereoRmsDb (r), aRms = stereoRmsDb (a);

        probe.prepare (fs, 256);
        std::printf ("  %-12s %7.1f / %6.1f %7.1f / %6.1f %8d %8.2f %7.1f\n", m.id.c_str(), rPeak, rRms, aPeak, aRms,
                     probe.getLatencySamples(), 100.0 * secs / (riff.getNumSamples() / fs), probe.getTailSeconds());

        // All amps should sit at a similar loudness so switching between them doesn't jump.
        // (clean amps sit around -19 dB RMS with the riff, driven ones around -15 dB)
        check (rPeak < -0.5 && aPeak < -0.5, m.id + ": default peaks stay below -0.5 dBFS");
        check (rRms > -21.0 && rRms < -13.0, m.id + ": riff loudness in the collection's window (" + std::to_string (rRms) + " dB)");
        check (aRms > -24.0 && aRms < -13.0, m.id + ": arpeggio loudness in the collection's window (" + std::to_string (aRms) + " dB)");
    }

    std::printf ("  riff peak / rms over the Gain x Master range (all other controls at default):\n");
    std::printf ("  %-12s %11s %11s %11s %11s %11s %11s\n", "", "g0 m0", "g0 m10", "g5 m0", "g5 m10", "g10 m0", "g10 m10");
    for (const auto& m : allAmpModels())
    {
        std::printf ("  %-12s", m.id.c_str());
        double loudest = -200.0;
        for (float g : { 0.0f, 5.0f, 10.0f })
            for (float mv : { 0.0f, 10.0f })
            {
                EffectAmpEngine probe (m);
                auto p = probe.defaultParams();
                p.gain = g; p.master = mv;
                auto r = render (m, riff, fs, p);
                const double pk = std::max (peakDb (r.getReadPointer (0), r.getNumSamples()), peakDb (r.getReadPointer (1), r.getNumSamples()));
                loudest = std::max (loudest, pk);
                std::printf (" %5.1f/%5.1f", pk, stereoRmsDb (r));
            }
        std::printf ("\n");
        check (loudest < 3.0, m.id + ": peaks stay below +3 dBFS anywhere on Gain and Master");
    }
}

//==============================================================================
void testControlsDoSomething()
{
    section ("Every FX control changes the sound");
    const double fs = 48000.0;
    const auto arp = cleanArpeggio (fs);
    const int n = juce::jmin (arp.getNumSamples(), (int) (3.0 * fs));
    juce::AudioBuffer<float> input (1, n);
    input.copyFrom (0, 0, arp, 0, 0, n);

    for (const auto& m : allAmpModels())
    {
        EffectAmpEngine probe (m);
        const auto defaults = probe.defaultParams();

        for (size_t c = 0; c < m.fxControls.size(); ++c)
        {
            auto lo = defaults, hi = defaults;
            lo.fx[c] = m.fxControls[c].fromNormalised (0.0f);
            hi.fx[c] = m.fxControls[c].fromNormalised (1.0f);

            auto a = render (m, input, fs, lo, 256, 0.5);
            auto b = render (m, input, fs, hi, 256, 0.5);
            const double ref = juce::jmax (stereoRmsDb (a), stereoRmsDb (b));

            for (int ch = 0; ch < 2; ++ch)
                b.addFrom (ch, 0, a, ch, 0, a.getNumSamples(), -1.0f);
            const double diff = stereoRmsDb (b) - ref;

            check (diff > -30.0, m.id + ": '" + m.fxControls[c].id + "' changes the output (difference "
                                     + std::to_string ((int) diff) + " dB)");
        }
    }
}

//==============================================================================
void testGates()
{
    section ("Effect amp gates");
    const double fs = 48000.0;
    std::mt19937 rng (5);
    std::normal_distribution<float> nd (0.0f, 1.0f);
    // half a second of silence (the gate settles closed), then a second of pickup hiss at about -70 dBFS
    juce::AudioBuffer<float> hiss (1, (int) (1.5 * fs));
    hiss.clear();
    for (int i = (int) (0.5 * fs); i < hiss.getNumSamples(); ++i)
        hiss.setSample (0, i, 0.0003f * nd (rng));

    for (const auto& m : allAmpModels())
    {
        if (! m.voicing.hasGate)
            continue;

        EffectAmpEngine probe (m);
        auto p = probe.defaultParams();
        p.gain = 9.0f;
        p.gateDb = -90.0f;
        auto open = render (m, hiss, fs, p);
        p.gateDb = -45.0f;
        auto gated = render (m, hiss, fs, p);

        const int last = (int) (0.5 * fs), from = hiss.getNumSamples() - last;
        const double openDb = stereoRmsDb (open, from, last), gatedDb = stereoRmsDb (gated, from, last);
        check (gatedDb < openDb - 40.0, m.id + ": gate removes amplified hiss (" + std::to_string ((int) openDb)
                                            + " -> " + std::to_string ((int) gatedDb) + " dBFS)");
    }
}

} // namespace

void runEffectAmpTests()
{
    testModels();
    testStress();
    testSilence();
    testLevelsAndCpu();
    testControlsDoSomething();
    testGates();
}
