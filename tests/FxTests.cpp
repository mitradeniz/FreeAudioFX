// Tests for every effect in the oap_fx library (pedals, Pedalboard, effect amps).

#include "TestFramework.h"
#include "TestSignals.h"
#include "TestSuites.h"

#include "fx/Effect.h"
#include "fx/FxEngines.h"

#include <chrono>
#include <random>
#include <set>

using testing::check;
using testing::section;
using namespace oap::fx;

namespace
{

void randomise (Effect& fx, std::mt19937& rng)
{
    std::uniform_real_distribution<float> u (0.0f, 1.0f);
    const auto& specs = fx.info().params;
    for (size_t i = 0; i < specs.size(); ++i)
        fx.setParameter (i, specs[i].fromNormalised (u (rng)));
}

void testSpecs()
{
    section ("Effect registry and parameter specs");
    const auto& list = allEffects();
    check (list.size() == 20, "20 effects registered (got " + std::to_string (list.size()) + ")");

    int advanced = 0;
    std::set<std::string> ids;
    for (const auto* info : list)
    {
        advanced += info->advanced ? 1 : 0;
        check (ids.insert (info->id).second, "unique effect id " + info->id);
        check (! info->params.empty() && info->params.size() <= EffectInfo::maxParams, info->id + ": 1-6 parameters");
        check (createEffect (info->id) != nullptr, info->id + ": factory works");

        std::set<std::string> paramIds;
        for (const auto& p : info->params)
        {
            check (paramIds.insert (p.id).second, info->id + "." + p.id + " unique");
            check (p.def >= p.min && p.def <= p.max, info->id + "." + p.id + " default in range");
            check (! p.format (p.def).empty(), info->id + "." + p.id + " formats");
            const float roundTrip = p.fromNormalised (p.toNormalised (p.def));
            check (std::abs (roundTrip - p.def) < 0.01f * (p.max - p.min) + 1.0e-3f, info->id + "." + p.id + " normalisation round-trips");
        }
    }
    check (advanced == 8, "8 advanced effects (got " + std::to_string (advanced) + ")");
}

void testStress()
{
    section ("Effect stress test (random parameters, 44.1-96 kHz, odd block sizes, mono + stereo)");
    std::mt19937 rng (7);
    std::uniform_real_distribution<float> noise (-1.0f, 1.0f);

    for (const auto* info : allEffects())
    {
        bool finite = true;
        float peak = 0.0f;

        for (double fs : { 44100.0, 48000.0, 96000.0 })
            for (int channels : { 1, 2 })
                for (int block : { 1, 37, 512, 1500 })
                {
                    auto fx = createEffect (info->id);
                    fx->prepare (fs, std::min (block, 512));
                    std::vector<float> l ((size_t) block), r ((size_t) block);
                    float* chans[] = { l.data(), r.data() };
                    const int total = (int) (fs * (block == 1 ? 0.05 : 0.6));

                    for (int done = 0, b = 0; done < total; done += block, ++b)
                    {
                        if (b % 4 == 0)
                            randomise (*fx, rng);
                        for (int i = 0; i < block; ++i)
                        {
                            const double t = (done + i) / fs;
                            const float s = t < 0.3 ? 0.8f * (float) std::sin (2.0 * 3.14159265 * (80.0 + 3000.0 * t) * t)
                                                    : (t < 0.45 ? noise (rng) : 0.0f);
                            l[(size_t) i] = s;
                            r[(size_t) i] = -0.7f * s;
                        }
                        fx->process (chans, channels, block);
                        for (int c = 0; c < channels; ++c)
                            for (int i = 0; i < block; ++i)
                            {
                                const float v = chans[c][i];
                                finite = finite && std::isfinite (v);
                                peak = std::max (peak, std::abs (v));
                            }
                    }
                }

        check (finite, info->id + ": output stays finite");
        check (peak < 24.0f, info->id + ": output bounded (peak " + std::to_string (testing::db (peak)) + " dBFS)");
    }
}

void testSilence()
{
    section ("Effects decay to silence");
    for (const auto* info : allEffects())
    {
        auto fx = createEffect (info->id);
        const double fs = 48000.0;
        fx->prepare (fs, 256);
        std::vector<float> l (256), r (256);
        float* chans[] = { l.data(), r.data() };
        std::mt19937 rng (3);
        std::uniform_real_distribution<float> noise (-0.5f, 0.5f);

        const double tail = std::min (25.0, fx->getTailSeconds() + 2.0);
        const int blocks = (int) ((0.5 + tail) * fs / 256);
        double energy = 0.0;
        int measured = 0;
        for (int b = 0; b < blocks; ++b)
        {
            const bool loud = b * 256 < fs * 0.5;
            for (int i = 0; i < 256; ++i)
                l[(size_t) i] = r[(size_t) i] = loud ? noise (rng) : 0.0f;
            fx->process (chans, 2, 256);
            if (b >= blocks - (int) (0.25 * fs / 256))
            {
                for (int i = 0; i < 256; ++i)
                    energy += (double) l[(size_t) i] * l[(size_t) i] + (double) r[(size_t) i] * r[(size_t) i];
                measured += 512;
            }
        }
        const double level = 10.0 * std::log10 (energy / std::max (1, measured) + 1.0e-30);
        check (level < -90.0, info->id + ": silent after its tail (" + std::to_string ((int) level) + " dBFS)");
    }
}

void testLevelsAndCpu()
{
    section ("Default-setting levels with a guitar DI (mono in, stereo out) and CPU");
    const double fs = 48000.0;
    auto riff = testsignals::heavyRiff (fs);
    const int n = riff.getNumSamples();
    const double inRms = testsignals::rmsDb (riff.getReadPointer (0), n);
    std::printf ("  input: %.1f dBFS RMS\n", inRms);
    std::printf ("  %-16s %10s %10s %9s %8s\n", "effect", "rms dB", "change", "latency", "cpu %");

    for (const auto* info : allEffects())
    {
        auto fx = createEffect (info->id);
        fx->prepare (fs, 256);
        std::vector<float> l ((size_t) n), r ((size_t) n);
        std::copy (riff.getReadPointer (0), riff.getReadPointer (0) + n, l.begin());
        std::copy (l.begin(), l.end(), r.begin());

        const auto t0 = std::chrono::steady_clock::now();
        for (int start = 0; start < n; start += 256)
        {
            float* chans[] = { l.data() + start, r.data() + start };
            fx->process (chans, 2, std::min (256, n - start));
        }
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();

        const double outRms = 10.0 * std::log10 ((std::pow (10.0, testsignals::rmsDb (l.data(), n) / 10.0)
                                                  + std::pow (10.0, testsignals::rmsDb (r.data(), n) / 10.0)) / 2.0);
        std::printf ("  %-16s %10.1f %+10.1f %9d %8.2f\n", info->name.c_str(), outRms, outRms - inRms,
                     fx->getLatencySamples(), 100.0 * secs / (n / fs));
        check (std::abs (outRms - inRms) < 16.0, info->id + ": default level within 16 dB of the input");
    }
}

double zeroCrossingHz (const std::vector<float>& x, int start, double fs)
{
    int crossings = 0, first = -1, last = -1;
    for (size_t i = (size_t) start + 1; i < x.size(); ++i)
        if (x[i - 1] <= 0.0f && x[i] > 0.0f)
        {
            if (first < 0) first = (int) i;
            last = (int) i;
            ++crossings;
        }
    return crossings > 1 ? (crossings - 1) * fs / (last - first) : 0.0;
}

void testBuildingBlocks()
{
    section ("Effect building blocks");
    const double fs = 48000.0;

    for (int factor : { 2, 4 })
    {
        Oversampler os;
        os.prepare (factor, 64);
        const int latency = os.getLatency();
        std::vector<float> in (4800), out (4800);
        for (size_t i = 0; i < in.size(); ++i)
            in[i] = 0.5f * (float) std::sin (2.0 * 3.14159265358979 * 1000.0 * (double) i / fs);
        for (size_t start = 0; start < in.size(); start += 64)
        {
            os.up (in.data() + start, 64);
            os.down (out.data() + start, 64);
        }
        double err = 0.0, sig = 0.0;
        for (size_t i = 1000; i < in.size(); ++i)
        {
            const double e = out[i] - in[i - (size_t) latency];
            err += e * e;
            sig += (double) in[i] * in[i];
        }
        const double snr = 10.0 * std::log10 (sig / (err + 1.0e-30));
        std::printf ("  %dx oversampler: latency %d samples, passband error %.1f dB\n", factor, latency, -snr);
        check (snr > 60.0, std::to_string (factor) + "x oversampler is transparent and its latency is exact");
    }

    {
        PitchShifterCore shifter;
        shifter.prepare (fs, 45.0f);
        std::vector<float> out ((size_t) fs);
        for (size_t i = 0; i < out.size(); ++i)
            out[i] = shifter.process ((float) std::sin (2.0 * 3.14159265358979 * 220.0 * (double) i / fs), 1.4983f);
        const double hz = zeroCrossingHz (out, 4800, fs);
        std::printf ("  pitch shifter: 220 Hz up a fifth -> %.1f Hz\n", hz);
        check (std::abs (hz - 329.6) < 4.0, "pitch shifter lands on the right note");
    }

    {
        auto delay = createEffect ("delay");
        delay->prepare (fs, 512);
        delay->setParameter (0, 250.0f);  // time
        delay->setParameter (2, 0.0f);    // feedback
        delay->setParameter (3, 50.0f);   // mix: dry and echo both at unity
        delay->reset();
        std::vector<float> x ((size_t) fs, 0.0f);
        x[100] = 1.0f;
        float* chans[] = { x.data() };
        for (size_t start = 0; start < x.size(); start += 512)
        {
            float* c[] = { chans[0] + start };
            delay->process (c, 1, (int) std::min<size_t> (512, x.size() - start));
        }
        size_t best = 0;
        for (size_t i = 200; i < x.size(); ++i)
            if (std::abs (x[i]) > std::abs (x[best])) best = i;
        const double ms = (double) (best - 100) * 1000.0 / fs;
        std::printf ("  delay: echo after %.1f ms (expected 250)\n", ms);
        check (std::abs (ms - 250.0) < 1.0, "delay time is accurate");
    }
}

} // namespace

void runFxTests()
{
    testBuildingBlocks();
    testSpecs();
    testStress();
    testSilence();
    testLevelsAndCpu();
}
