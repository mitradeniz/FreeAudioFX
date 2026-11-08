// DSP test runner for the Open Audio Plugins amps.
//
//   oap_dsp_tests                 run all checks (exit code != 0 on failure)
//   oap_dsp_tests --render <dir>  also write demo WAV files through both amps

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

#include "CleanAmpEngine.h"
#include "DriveAmpEngine.h"
#include "TestFramework.h"
#include "TestSuites.h"
#include "TestSignals.h"

#include <chrono>
#include <cstdio>
#include <functional>
#include <random>

using namespace oap::dsp;
using namespace testsignals;

namespace
{
using testing::section;
using testing::db;

void check (bool ok, const juce::String& what) { testing::check (ok, what.toStdString()); }

//==============================================================================
template <typename Engine>
void runEngine (Engine& engine, juce::AudioBuffer<float>& io, int inputChannels, int blockSize,
                const typename Engine::Params& params)
{
    for (int start = 0; start < io.getNumSamples(); start += blockSize)
    {
        const int len = juce::jmin (blockSize, io.getNumSamples() - start);
        juce::AudioBuffer<float> view (io.getArrayOfWritePointers(), io.getNumChannels(), start, len);
        engine.process (view, inputChannels, params);
    }
}

template <typename Engine>
juce::AudioBuffer<float> processMono (const juce::AudioBuffer<float>& input, double fs,
                                      const typename Engine::Params& params, int outChannels = 1,
                                      int blockSize = 256)
{
    Engine engine;
    engine.prepare (fs, blockSize);
    juce::AudioBuffer<float> io (outChannels, input.getNumSamples());
    io.clear();
    io.copyFrom (0, 0, input, 0, 0, input.getNumSamples());
    runEngine (engine, io, 1, blockSize, params);
    return io;
}

juce::AudioBuffer<float> sine (double fs, double hz, double seconds, float amplitude)
{
    juce::AudioBuffer<float> b (1, (int) (fs * seconds));
    for (int i = 0; i < b.getNumSamples(); ++i)
        b.setSample (0, i, amplitude * (float) std::sin (2.0 * kPi * hz * i / fs));
    return b;
}

//==============================================================================
void testBiquads()
{
    section ("Biquad designs");
    const double fs = 48000.0;

    auto lp = BiquadCoeffs::lowpass (fs, 1000.0, 0.707);
    check (std::abs (lp.magnitudeAt (1.0, fs) - 1.0) < 1e-3, "lowpass DC gain is unity");
    check (std::abs (db (lp.magnitudeAt (1000.0, fs)) + 3.01) < 0.1, "lowpass -3 dB at cutoff");
    check (lp.magnitudeAt (23999.0, fs) < 1e-3, "lowpass rejects Nyquist");

    auto hp = BiquadCoeffs::highpass (fs, 100.0, 0.707);
    check (hp.magnitudeAt (1.0, fs) < 1e-3, "highpass rejects DC");
    check (std::abs (hp.magnitudeAt (10000.0, fs) - 1.0) < 1e-3, "highpass passes HF");

    auto pk = BiquadCoeffs::peak (fs, 2000.0, 1.0, 6.0);
    check (std::abs (db (pk.magnitudeAt (2000.0, fs)) - 6.0) < 0.05, "peak gain at centre");

    auto ls = BiquadCoeffs::lowShelf (fs, 200.0, 0.707, -9.0);
    check (std::abs (db (ls.magnitudeAt (5.0, fs)) + 9.0) < 0.1, "low shelf gain at DC");
    check (std::abs (db (ls.magnitudeAt (15000.0, fs))) < 0.1, "low shelf flat at HF");

    auto hs = BiquadCoeffs::highShelf (fs, 3000.0, 0.707, 8.0);
    check (std::abs (db (hs.magnitudeAt (20000.0, fs)) - 8.0) < 0.3, "high shelf gain at HF");
    check (std::abs (db (hs.magnitudeAt (20.0, fs))) < 0.05, "high shelf flat at LF");
}

void testToneStack()
{
    section ("Tone stack (Yeh/Smith TMB model)");
    const double fs = 192000.0;

    const std::pair<const char*, ToneStack::Components> presets[] = {
        { "bassman", ToneStack::bassman() },
        { "british", ToneStack::british() },
        { "blackface", ToneStack::blackface() },
    };

    for (const auto& [name, comps] : presets)
    {
        ToneStack ts;
        ts.prepare (fs, comps);
        double maxGain = 0.0;

        for (double b : { 0.0, 0.25, 0.5, 0.75, 1.0 })
            for (double m : { 0.0, 0.25, 0.5, 0.75, 1.0 })
                for (double t : { 0.0, 0.25, 0.5, 0.75, 1.0 })
                {
                    ts.setControls (b, m, t);
                    for (double f = 20.0; f < 20000.0; f *= 1.05)
                        maxGain = std::max (maxGain, ts.magnitudeAt (f));
                }

        check (maxGain <= 1.0 + 1e-6, juce::String (name) + ": passive network never exceeds 0 dB (max "
                                          + juce::String (db (maxGain), 2) + " dB)");

        ts.setControls (0.5, 0.5, 0.0);
        const double trebleLow = ts.magnitudeAt (5000.0);
        ts.setControls (0.5, 0.5, 1.0);
        check (ts.magnitudeAt (5000.0) > trebleLow * 1.5, juce::String (name) + ": treble raises 5 kHz");

        ts.setControls (0.0, 0.5, 0.5);
        const double bassLow = ts.magnitudeAt (80.0);
        ts.setControls (1.0, 0.5, 0.5);
        check (ts.magnitudeAt (80.0) > bassLow * 1.5, juce::String (name) + ": bass raises 80 Hz");

        ts.setControls (0.5, 0.0, 0.5);
        const double midLow = ts.magnitudeAt (600.0);
        ts.setControls (0.5, 1.0, 0.5);
        check (ts.magnitudeAt (600.0) > midLow * 1.2, juce::String (name) + ": middle raises 600 Hz");

        ts.setControls (0.5, 0.5, 0.5);
        std::printf ("  %-10s noon response: 100Hz %6.1f dB | 500Hz %6.1f dB | 1k %6.1f dB | 3k %6.1f dB\n", name,
                     db (ts.magnitudeAt (100)), db (ts.magnitudeAt (500)), db (ts.magnitudeAt (1000)),
                     db (ts.magnitudeAt (3000)));
    }
}

void testCabinetModels()
{
    section ("Cabinet voicings");
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        for (auto v : { CabinetModel::Voicing::Closed4x12, CabinetModel::Voicing::Open2x12, CabinetModel::Voicing::Combo1x12 })
        {
            CabinetModel m;
            m.prepare (fs, v);
            const double pass = m.magnitudeAt (1500.0);
            check (std::isfinite (pass) && pass > 0.3 && pass < 4.0, "cab passband sane");
            check (m.magnitudeAt (15000.0) < pass * 0.02, "cab rolls off above 15 kHz (< -34 dB)");
            check (m.magnitudeAt (30.0) < pass * 0.3, "cab rolls off below 30 Hz");
        }
    }
}

//==============================================================================
template <typename Engine, typename Randomise>
void stressEngine (const char* name, Randomise randomise)
{
    section ((juce::String ("Stress: ") + name).toRawUTF8());
    std::mt19937 rng (42);

    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        for (int block : { 1, 17, 64, 480, 2048 })
        {
            const int prepared = block == 2048 ? 512 : block; // also test hosts exceeding the prepared size
            Engine engine;
            engine.prepare (fs, prepared);

            const int total = (int) (fs * (block == 1 ? 0.25 : 1.5));
            juce::AudioBuffer<float> buf (2, total);
            std::uniform_real_distribution<float> noise (-1.0f, 1.0f);

            for (int i = 0; i < total; ++i)
            {
                const double t = i / fs;
                float x = 0.0f;
                if (t < 0.3)       x = 0.9f * (float) std::sin (2.0 * kPi * (50.0 + 8000.0 * t) * t); // loud sweep
                else if (t < 0.6)  x = noise (rng);                                                     // full-scale noise
                else if (t < 0.7)  x = (i % 4000 == 0) ? 1.0f : 0.0f;                                   // impulses
                else if (t < 1.0)  x = 0.0f;                                                            // silence
                else               x = 0.05f * noise (rng);
                buf.setSample (0, i, x);
                buf.setSample (1, i, -x * 0.5f);
            }

            bool finite = true;
            float peak = 0.0f;
            typename Engine::Params params;

            for (int start = 0; start < total; start += block)
            {
                const int len = juce::jmin (block, total - start);
                if ((start / block) % 3 == 0)
                    randomise (params, rng);

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

            check (finite, juce::String (name) + " output finite @" + juce::String (fs) + " block " + juce::String (block));
            check (peak < 16.0f, juce::String (name) + " output bounded (peak " + juce::String (db (peak), 1) + " dBFS) @"
                                     + juce::String (fs) + " block " + juce::String (block));
        }
    }
    std::printf ("  ran 4 sample rates x 5 block sizes with randomised parameters\n");
}

template <typename Engine>
void testSilence (const char* name, typename Engine::Params params, double settleSeconds)
{
    const double fs = 48000.0;
    auto input = sine (fs, 110.0, 1.0, 0.5f);
    juce::AudioBuffer<float> sig (1, (int) (fs * (1.0 + settleSeconds + 0.5)));
    sig.clear();
    sig.copyFrom (0, 0, input, 0, 0, input.getNumSamples());

    auto out = processMono<Engine> (sig, fs, params, 2);
    const int tail = (int) (fs * 0.5);
    const double level = rmsDb (out.getReadPointer (0, out.getNumSamples() - tail), tail);
    check (level < -100.0, juce::String (name) + ": decays to silence (" + juce::String (level, 1) + " dBFS)");
}

void testGate()
{
    section ("Noise gate");
    const double fs = 48000.0;
    std::mt19937 rng (3);
    std::normal_distribution<float> n (0.0f, 1.0f);
    juce::AudioBuffer<float> hiss (1, (int) fs);
    for (int i = 0; i < hiss.getNumSamples(); ++i)
        hiss.setSample (0, i, 0.0003f * n (rng)); // ~ -70 dBFS pickup hiss

    DriveAmpEngine::Params p;
    p.gain = 9.0f;
    p.gateDb = -90.0f;
    auto open = processMono<DriveAmpEngine> (hiss, fs, p);
    p.gateDb = -45.0f;
    auto gated = processMono<DriveAmpEngine> (hiss, fs, p);

    const int half = hiss.getNumSamples() / 2;
    const double openDb = rmsDb (open.getReadPointer (0, half), half);
    const double gatedDb = rmsDb (gated.getReadPointer (0, half), half);
    std::printf ("  hiss at gain 9: gate off %.1f dBFS, gate -45 dB %.1f dBFS\n", openDb, gatedDb);
    check (gatedDb < openDb - 40.0, "gate removes amplified hiss");

    // the gate must still let a played note through
    auto note = sine (fs, 196.0, 0.5, 0.1f);
    auto played = processMono<DriveAmpEngine> (note, fs, p);
    check (rmsDb (played.getReadPointer (0, 4800), 19200) > -30.0, "gate opens for a played note");
}

//==============================================================================
void testLevels()
{
    section ("Output levels with a guitar-like DI signal");
    const double fs = 48000.0;
    auto riff = heavyRiff (fs);
    auto arp = cleanArpeggio (fs);
    std::printf ("  DI riff: peak %.1f dBFS, rms %.1f dBFS | DI arpeggio: peak %.1f, rms %.1f\n",
                 peakDb (riff.getReadPointer (0), riff.getNumSamples()), rmsDb (riff.getReadPointer (0), riff.getNumSamples()),
                 peakDb (arp.getReadPointer (0), arp.getNumSamples()), rmsDb (arp.getReadPointer (0), arp.getNumSamples()));

    std::printf ("  Drive Amp (riff)     gain   master  boost  ->  peak / rms dBFS\n");
    for (float g : { 0.0f, 3.0f, 6.5f, 10.0f })
        for (float m : { 0.0f, 5.0f, 10.0f })
        {
            DriveAmpEngine::Params p;
            p.gain = g; p.master = m; p.gateDb = -90.0f;
            auto out = processMono<DriveAmpEngine> (riff, fs, p);
            const double rms = rmsDb (out.getReadPointer (0), out.getNumSamples());
            const double pk = peakDb (out.getReadPointer (0), out.getNumSamples());
            std::printf ("                       %5.1f  %5.1f    off   ->  %6.1f / %6.1f\n", g, m, pk, rms);
            check (pk < 0.0, "drive defaults-ish stay below 0 dBFS");
        }

    {
        DriveAmpEngine::Params p;
        p.boost = true;
        auto out = processMono<DriveAmpEngine> (riff, fs, p);
        std::printf ("                       %5.1f  %5.1f    on    ->  %6.1f / %6.1f\n", p.gain, p.master,
                     peakDb (out.getReadPointer (0), out.getNumSamples()), rmsDb (out.getReadPointer (0), out.getNumSamples()));
    }

    std::printf ("  Clean Amp (arpeggio) volume master bright ->  peak / rms dBFS\n");
    for (float v : { 0.0f, 3.0f, 4.5f, 7.0f, 10.0f })
        for (float m : { 0.0f, 5.0f, 10.0f })
        {
            CleanAmpEngine::Params p;
            p.volume = v; p.master = m; p.reverb = 0.0f;
            auto out = processMono<CleanAmpEngine> (arp, fs, p);
            const double pk = peakDb (out.getReadPointer (0), out.getNumSamples());
            std::printf ("                       %5.1f  %5.1f    off   ->  %6.1f / %6.1f\n", v, m, pk,
                         rmsDb (out.getReadPointer (0), out.getNumSamples()));
            check (pk < 0.0, "clean output stays below 0 dBFS");
        }
}

/** Measures how "clean" the clean amp is and how much the drive amp distorts. */
void testDistortionAmount()
{
    section ("Harmonic distortion (1 kHz-ish sine, cab off)");
    const double fs = 48000.0;
    const int fftOrder = 15, fftSize = 1 << fftOrder;
    const double f0 = 64.0 * fs / fftSize * 11.0; // bin-centred (~1031 Hz)

    auto thd = [&] (const juce::AudioBuffer<float>& out)
    {
        std::vector<float> data ((size_t) fftSize * 2, 0.0f);
        const float* x = out.getReadPointer (0, out.getNumSamples() - fftSize);
        for (int i = 0; i < fftSize; ++i)
            data[(size_t) i] = x[i] * (float) (0.5 - 0.5 * std::cos (2.0 * kPi * i / fftSize));
        juce::dsp::FFT fft (fftOrder);
        fft.performFrequencyOnlyForwardTransform (data.data());
        const int bin0 = (int) std::lround (f0 * fftSize / fs);
        auto power = [&] (int bin) { double p = 0; for (int k = bin - 2; k <= bin + 2; ++k) p += (double) data[(size_t) k] * data[(size_t) k]; return p; };
        double harm = 0.0;
        for (int h = 2; h * bin0 + 2 < fftSize / 2; ++h)
            harm += power (h * bin0);
        return 100.0 * std::sqrt (harm / power (bin0));
    };

    auto in = sine (fs, f0, 2.0, 0.1f); // -20 dBFS, a typical single-note DI level

    for (float v : { 2.0f, 4.5f, 7.0f, 10.0f })
    {
        CleanAmpEngine::Params p;
        p.volume = v; p.reverb = 0.0f; p.cabinet = CabinetSection::Off;
        const double t = thd (processMono<CleanAmpEngine> (in, fs, p));
        std::printf ("  Clean volume %4.1f : THD %6.2f %%\n", v, t);
        if (v <= 4.5f)
            check (t < 5.0, "clean amp stays clean at moderate volume");
        if (v >= 10.0f)
            check (t > 5.0, "clean amp breaks up when the volume is maxed");
    }

    for (float g : { 0.0f, 3.0f, 6.5f, 10.0f })
    {
        DriveAmpEngine::Params p;
        p.gain = g; p.gateDb = -90.0f; p.cabinet = CabinetSection::Off;
        const double t = thd (processMono<DriveAmpEngine> (in, fs, p));
        std::printf ("  Drive gain   %4.1f : THD %6.2f %%\n", g, t);
        if (g >= 6.5f)
            check (t > 30.0, "drive amp is heavily saturated at high gain");
    }
}

/** Returns how far (dB) the non-harmonic energy sits below the harmonic energy. */
double measureAliasing (int bin0, const DriveAmpEngine::Params& p)
{
    const double fs = 48000.0;
    const int fftOrder = 15, fftSize = 1 << fftOrder;
    const double f0 = bin0 * fs / fftSize; // bin-centred, so harmonics land exactly on bins

    auto out = processMono<DriveAmpEngine> (sine (fs, f0, 2.0, 0.25f), fs, p);

    std::vector<float> data ((size_t) fftSize * 2, 0.0f);
    const float* x = out.getReadPointer (0, out.getNumSamples() - fftSize);
    for (int i = 0; i < fftSize; ++i)
    {
        const double w = 2.0 * kPi * i / fftSize; // Blackman-Harris
        data[(size_t) i] = x[i] * (float) (0.35875 - 0.48829 * std::cos (w) + 0.14128 * std::cos (2 * w) - 0.01168 * std::cos (3 * w));
    }
    juce::dsp::FFT fft (fftOrder);
    fft.performFrequencyOnlyForwardTransform (data.data());

    std::vector<bool> isHarmonic ((size_t) fftSize / 2, false);
    for (int h = 1; h * bin0 < fftSize / 2; ++h)
        for (int k = h * bin0 - 4; k <= juce::jmin (fftSize / 2 - 1, h * bin0 + 4); ++k)
            isHarmonic[(size_t) k] = true;

    double harmonicPower = 0.0, aliasPower = 0.0;
    // Audible band only: the last few hundred Hz below Nyquist sit in the
    // decimation filter's transition band and are removed by the cabinet anyway.
    const int minBin = (int) (20.0 * fftSize / fs);
    const int maxBin = (int) (20000.0 * fftSize / fs);
    for (int k = minBin; k < maxBin; ++k)
    {
        const double pw = (double) data[(size_t) k] * data[(size_t) k];
        (isHarmonic[(size_t) k] ? harmonicPower : aliasPower) += pw;
    }

    return -10.0 * std::log10 (aliasPower / harmonicPower);
}

void testAliasing()
{
    section ("Aliasing (audible non-harmonic energy below the harmonics, cab off, -12 dBFS sine)");

    DriveAmpEngine::Params typical;
    typical.gateDb = -90.0f; typical.cabinet = CabinetSection::Off;

    DriveAmpEngine::Params extreme = typical;
    extreme.gain = 10.0f; extreme.master = 8.0f; extreme.treble = 10.0f; extreme.presence = 10.0f;

    const double typical1k = measureAliasing (887, typical);   // ~1.3 kHz: top of the guitar's range
    const double extreme1k = measureAliasing (887, extreme);
    const double extreme3k = measureAliasing (2253, extreme);  // ~3.3 kHz: torture test

    std::printf ("  1.3 kHz, default settings : %.1f dB\n", typical1k);
    std::printf ("  1.3 kHz, everything maxed : %.1f dB\n", extreme1k);
    std::printf ("  3.3 kHz, everything maxed : %.1f dB (torture test, info only)\n", extreme3k);

    check (typical1k > 60.0, "aliasing far below the signal at default settings");
    check (extreme1k > 50.0, "aliasing well below the signal even with everything maxed");
}

//==============================================================================
void testImpulseResponse()
{
    section ("User impulse response");
    const double fs = 48000.0;

    // Build a plausible cab-like IR: filtered, decaying noise.
    const int irLength = 2048;
    juce::AudioBuffer<float> ir (1, irLength);
    std::mt19937 rng (9);
    std::normal_distribution<float> n (0.0f, 1.0f);
    CabinetModel shape;
    shape.prepare (44100.0, CabinetModel::Voicing::Closed4x12);
    for (int i = 0; i < irLength; ++i)
        ir.setSample (0, i, (float) shape.process (i == 0 ? 1.0 : 0.0) + 0.002f * n (rng) * std::exp (-(float) i / 200.0f));

    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("oap_test_ir.wav");
    file.deleteFile();
    {
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (44100.0)
                                                                                  .withNumChannels (1)
                                                                                  .withBitsPerSample (24));
        check (writer != nullptr, "can write test IR");
        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer (ir, 0, irLength);
    }

    DriveAmpEngine engine;
    engine.prepare (fs, 256);
    const auto error = engine.getCabinet().loadImpulseResponse (file);
    check (error.isEmpty(), "IR loads: " + error);

    const auto bad = engine.getCabinet().loadImpulseResponse (juce::File ("/definitely/not/here.wav"));
    check (bad.isNotEmpty(), "missing IR reports an error");

    DriveAmpEngine::Params p;
    p.cabinet = CabinetSection::UserIR;
    p.gateDb = -90.0f;

    auto riff = heavyRiff (fs);
    juce::AudioBuffer<float> io (1, riff.getNumSamples());
    io.copyFrom (0, 0, riff, 0, 0, riff.getNumSamples());

    int waitedBlocks = 0;
    for (int start = 0; start < io.getNumSamples(); start += 256)
    {
        const int len = juce::jmin (256, io.getNumSamples() - start);
        juce::AudioBuffer<float> view (io.getArrayOfWritePointers(), 1, start, len);
        engine.process (view, 1, p);
        if (engine.getCabinet().getActiveType() != CabinetSection::UserIR)
        {
            ++waitedBlocks;
            juce::Thread::sleep (1); // give the background loader time
        }
    }

    check (engine.getCabinet().getActiveType() == CabinetSection::UserIR, "user IR becomes active");
    std::printf ("  IR became active after %d blocks\n", waitedBlocks);

    p.cabinet = CabinetSection::Closed4x12;
    auto reference = processMono<DriveAmpEngine> (riff, fs, p);
    const int tailStart = io.getNumSamples() / 2;
    const double irLevel = rmsDb (io.getReadPointer (0, tailStart), io.getNumSamples() - tailStart);
    const double cabLevel = rmsDb (reference.getReadPointer (0, tailStart), io.getNumSamples() - tailStart);
    std::printf ("  level with IR %.1f dBFS vs built-in 4x12 %.1f dBFS\n", irLevel, cabLevel);
    check (std::abs (irLevel - cabLevel) < 6.0, "user IR is level-matched to the built-in cabs");
    file.deleteFile();
}

void testCabSwitchIsClickFree()
{
    section ("Cabinet switching crossfade");
    const double fs = 48000.0;
    DriveAmpEngine engine;
    engine.prepare (fs, 128);
    auto in = sine (fs, 220.0, 1.0, 0.1f);
    juce::AudioBuffer<float> io (1, in.getNumSamples());
    io.copyFrom (0, 0, in, 0, 0, in.getNumSamples());
    DriveAmpEngine::Params p;
    p.gateDb = -90.0f;

    float maxJump = 0.0f, prev = 0.0f, typicalJump = 0.0f;
    for (int start = 0, blockIndex = 0; start < io.getNumSamples(); start += 128, ++blockIndex)
    {
        p.cabinet = 1 + (blockIndex / 40) % 3; // switch every ~100 ms
        juce::AudioBuffer<float> view (io.getArrayOfWritePointers(), 1, start, 128);
        engine.process (view, 1, p);
        for (int i = 0; i < 128; ++i)
        {
            const float v = view.getSample (0, i);
            const float jump = std::abs (v - prev);
            if (start > 4800) { maxJump = std::max (maxJump, jump); typicalJump = std::max (typicalJump * 0.9999f, jump); }
            prev = v;
        }
    }
    std::printf ("  largest sample-to-sample step while switching: %.4f\n", maxJump);
    check (maxJump < 0.5f, "no discontinuities when switching cabinets");
}

//==============================================================================
template <typename Engine>
void benchmark (const char* name, typename Engine::Params params)
{
    const double fs = 48000.0;
    const int block = 256;
    Engine engine;
    engine.prepare (fs, block);
    auto riff = heavyRiff (fs);
    juce::AudioBuffer<float> io (2, riff.getNumSamples());

    const auto t0 = std::chrono::steady_clock::now();
    const int passes = 3;
    for (int pass = 0; pass < passes; ++pass)
    {
        io.copyFrom (0, 0, riff, 0, 0, riff.getNumSamples());
        io.copyFrom (1, 0, riff, 0, 0, riff.getNumSamples());
        runEngine (engine, io, 2, block, params);
    }
    const double elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    const double audio = passes * riff.getNumSamples() / fs;
    std::printf ("  %-10s %.2f %% of one core at 48 kHz (%.0fx realtime), latency %d samples\n", name,
                 100.0 * elapsed / audio, audio / elapsed, engine.getLatencySamples());
}

//==============================================================================
void writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer, double fs)
{
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (fs)
                                                                              .withNumChannels (buffer.getNumChannels())
                                                                              .withBitsPerSample (24));
    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
}

void renderDemos (const juce::File& dir)
{
    section ("Rendering demo files");
    dir.createDirectory();
    const double fs = 48000.0;
    auto riff = heavyRiff (fs);
    auto arp = cleanArpeggio (fs);

    writeWav (dir.getChildFile ("00_di_riff.wav"), riff, fs);
    writeWav (dir.getChildFile ("00_di_arpeggio.wav"), arp, fs);

    struct DriveDemo { const char* file; std::function<void (DriveAmpEngine::Params&)> setup; };
    const DriveDemo driveDemos[] = {
        { "drive_01_default.wav",       [] (auto&) {} },
        { "drive_02_boost_tight.wav",   [] (auto& p) { p.boost = true; p.tight = 7.0f; p.gain = 7.5f; } },
        { "drive_03_crunch.wav",        [] (auto& p) { p.gain = 2.5f; p.middle = 7.0f; p.master = 6.0f; } },
        { "drive_04_scooped.wav",       [] (auto& p) { p.gain = 8.0f; p.bass = 7.0f; p.middle = 2.0f; p.treble = 7.0f; p.resonance = 6.0f; } },
        { "drive_05_2x12.wav",          [] (auto& p) { p.cabinet = CabinetSection::Open2x12; } },
    };

    for (const auto& d : driveDemos)
    {
        DriveAmpEngine::Params p;
        d.setup (p);
        writeWav (dir.getChildFile (d.file), processMono<DriveAmpEngine> (riff, fs, p, 2), fs);
        std::printf ("  wrote %s\n", d.file);
    }

    struct CleanDemo { const char* file; std::function<void (CleanAmpEngine::Params&)> setup; };
    const CleanDemo cleanDemos[] = {
        { "clean_01_default.wav",       [] (auto&) {} },
        { "clean_02_bright_reverb.wav", [] (auto& p) { p.bright = true; p.volume = 3.5f; p.reverb = 5.0f; } },
        { "clean_03_edge_of_breakup.wav", [] (auto& p) { p.volume = 8.0f; p.master = 7.0f; p.reverb = 2.0f; } },
        { "clean_04_1x12_dry.wav",      [] (auto& p) { p.cabinet = CabinetSection::Combo1x12; p.reverb = 0.0f; } },
    };

    for (const auto& d : cleanDemos)
    {
        CleanAmpEngine::Params p;
        d.setup (p);
        writeWav (dir.getChildFile (d.file), processMono<CleanAmpEngine> (arp, fs, p, 2), fs);
        std::printf ("  wrote %s\n", d.file);
    }
}

} // namespace


//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedNoDenormals noDenormals;

    testBiquads();
    testToneStack();
    testCabinetModels();

    stressEngine<DriveAmpEngine> ("Drive Amp", [] (DriveAmpEngine::Params& p, std::mt19937& rng)
    {
        std::uniform_real_distribution<float> k (0.0f, 10.0f);
        p.gain = k (rng); p.tight = k (rng); p.bass = k (rng); p.middle = k (rng); p.treble = k (rng);
        p.presence = k (rng); p.resonance = k (rng); p.master = k (rng);
        p.outputDb = k (rng) * 2.4f - 12.0f;
        p.gateDb = -90.0f + 6.0f * k (rng);
        p.boost = k (rng) > 5.0f;
        p.cabinet = (int) (k (rng) / 2.5f); // 0..3 (and 4 = user IR fallback occasionally)
    });

    stressEngine<CleanAmpEngine> ("Clean Amp", [] (CleanAmpEngine::Params& p, std::mt19937& rng)
    {
        std::uniform_real_distribution<float> k (0.0f, 10.0f);
        p.volume = k (rng); p.bass = k (rng); p.middle = k (rng); p.treble = k (rng);
        p.master = k (rng); p.reverb = k (rng);
        p.outputDb = k (rng) * 2.4f - 12.0f;
        p.bright = k (rng) > 5.0f;
        p.cabinet = (int) (k (rng) / 2.0f);
    });

    section ("Silence / denormals");
    {
        DriveAmpEngine::Params dp; dp.gateDb = -90.0f;
        testSilence<DriveAmpEngine> ("Drive Amp", dp, 1.0);
        CleanAmpEngine::Params cp; cp.reverb = 10.0f;
        testSilence<CleanAmpEngine> ("Clean Amp (reverb max)", cp, 8.0);
    }

    testGate();
    testLevels();
    testDistortionAmount();
    testAliasing();
    testImpulseResponse();
    testCabSwitchIsClickFree();

    section ("CPU");
    benchmark<DriveAmpEngine> ("Drive Amp", {});
    benchmark<CleanAmpEngine> ("Clean Amp", {});

    for (int i = 1; i + 1 < argc; ++i)
        if (juce::String (argv[i]) == "--render")
            renderDemos (juce::File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]));

    runFxTests();
    runEffectAmpTests();

    std::printf ("\n%d checks, %d failures\n", testing::checks, testing::failures);
    return testing::failures == 0 ? 0 : 1;
}
