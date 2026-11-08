// Filter, pitch and character pedals: Wah, Envelope Filter, Pitch Shifter,
// Guitar Synth, Lo-Fi.

#include "FxEngines.h"
#include "Registry.h"

#include <array>

namespace oap::fx
{
namespace
{

//==============================================================================
class Wah final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "wah", "Wah", "Filter",
            "Classic vocal wah. Automate the pedal, or let an LFO or your picking move it.",
            0xff3a3a3f, false,
            {
                ParamSpec::range ("pedal", "Pedal", 0.0f, 100.0f, 50.0f, "%", 0),
                ParamSpec::choice ("mode", "Mode", { "Manual", "Auto", "Envelope" }, 0),
                ParamSpec::range ("speed", "Speed", 0.1f, 8.0f, 1.5f, "Hz", 2, 1.5f),
                ParamSpec::knob ("sensitivity", "Sensitivity", 5.0f),
                ParamSpec::knob ("resonance", "Resonance", 6.0f),
            } };
        return info;
    }

    Wah() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        position.prepare (fs, 12.0);
        follower.prepare (fs, 4.0, 150.0);
        reset();
    }

    void reset() override
    {
        for (auto& f : filters) f.reset();
        follower.reset();
        position.reset (p (0) / 100.0f);
        lfo.setPhase (0.0);
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const int mode = choiceIndex (1);
        lfo.setRate (p (2), fs);
        const float q = 2.0f + p (4);
        const float sens = dbToGain (p (3) * 3.0f) * 4.0f;

        for (int i = 0; i < n; ++i)
        {
            float target = p (0) / 100.0f;
            if (mode == 1)
                target = 0.5f + 0.5f * lfo.sine();
            else if (mode == 2)
            {
                float peak = 0.0f;
                for (int c = 0; c < numCh; ++c) peak = std::max (peak, std::abs (ch[c][i]));
                target = clamp01 ((float) follower.process (peak) * sens);
            }
            lfo.advance();

            const float pos = position.next (target);
            if ((i & 7) == 0)
                for (int c = 0; c < numCh; ++c)
                    filters[(size_t) c].set (expMap (pos, 350.0f, 2300.0f), q, fs);

            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                ch[c][i] = 3.0f * filters[(size_t) c].bandpass (x) + 0.15f * x;
            }
        }
    }

private:
    double fs = 48000.0;
    std::array<Svf, 2> filters;
    Smoother position;
    oap::dsp::EnvelopeFollower follower;
    Lfo lfo;
};

//==============================================================================
class EnvelopeFilter final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "envfilter", "Envelope Filter", "Filter",
            "Funky auto-wah that sweeps with your picking dynamics, up or down.",
            0xffe8c21a, true,
            {
                ParamSpec::knob ("sensitivity", "Sensitivity", 6.0f),
                ParamSpec::knob ("range", "Range", 3.0f),
                ParamSpec::knob ("resonance", "Resonance", 6.0f),
                ParamSpec::choice ("mode", "Mode", { "Low-pass", "Band-pass", "High-pass" }, 0),
                ParamSpec::choice ("direction", "Direction", { "Up", "Down" }, 0),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 100.0f, "%", 0),
            } };
        return info;
    }

    EnvelopeFilter() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        follower.prepare (fs, 3.0, 120.0);
        reset();
    }

    void reset() override
    {
        for (auto& f : filters) f.reset();
        follower.reset();
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const float sens = dbToGain (p (0) * 4.0f) * 3.0f;
        const float start = expMap (p (1) / 10.0f, 150.0f, 1200.0f);
        const float q = 0.8f + p (2) * 1.1f;
        const int mode = choiceIndex (3);
        const bool down = choiceIndex (4) == 1;
        float dryG, wetG;
        MixLaw::equalPower (p (5) / 100.0f, dryG, wetG);
        if (p (5) >= 99.5f) { dryG = 0.0f; wetG = 1.0f; }

        for (int i = 0; i < n; ++i)
        {
            float peak = 0.0f;
            for (int c = 0; c < numCh; ++c) peak = std::max (peak, std::abs (ch[c][i]));
            const float env = clamp01 ((float) follower.process (peak) * sens);
            const float octaves = 4.5f * (down ? 1.0f - env : env);

            if ((i & 7) == 0)
                for (int c = 0; c < numCh; ++c)
                    filters[(size_t) c].set (start * std::pow (2.0f, octaves), q, fs);

            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                float lp, bp, hp;
                filters[(size_t) c].process (x, lp, bp, hp);
                const float wet = 0.7f * (mode == 0 ? lp : (mode == 1 ? bp * filters[(size_t) c].getK() * 1.5f : hp));
                ch[c][i] = x * dryG + wet * wetG;
            }
        }
    }

private:
    double fs = 48000.0;
    std::array<Svf, 2> filters;
    oap::dsp::EnvelopeFollower follower;
};

//==============================================================================
class PitchShifter final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "pitch", "Pitch Shifter", "Pitch & Synth",
            "Octaves, harmonies and whammy-style bends. Automate Bend for dive bombs.",
            0xffc2185b, true,
            {
                ParamSpec::range ("interval", "Interval", -24.0f, 24.0f, 12.0f, "st", 0),
                ParamSpec::range ("fine", "Detune", -50.0f, 50.0f, 0.0f, "ct", 0),
                ParamSpec::range ("bend", "Bend", 0.0f, 100.0f, 100.0f, "%", 0),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 50.0f, "%", 0),
                ParamSpec::knob ("tone", "Tone", 7.0f),
            } };
        return info;
    }

    PitchShifter() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& s : shifters) s.prepare (fs, 45.0f);
        ratioSmooth.prepare (fs, 25.0);
        reset();
    }

    void reset() override
    {
        for (auto& s : shifters) s.reset();
        for (auto& t : tone) t.reset();
        ratioSmooth.reset (targetSemitones());
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        for (auto& t : tone) t.set (expMap (p (4) / 10.0f, 1500.0f, 18000.0f), fs);
        float dryG, wetG;
        MixLaw::equalPower (p (3) / 100.0f, dryG, wetG);
        if (p (3) >= 99.5f) { dryG = 0.0f; wetG = 1.0f; }

        for (int i = 0; i < n; ++i)
        {
            const float ratio = std::pow (2.0f, ratioSmooth.next (targetSemitones()) / 12.0f);
            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                const float wet = tone[(size_t) c].process (shifters[(size_t) c].process (x, ratio));
                ch[c][i] = x * dryG + wet * wetG;
            }
        }
    }

private:
    float targetSemitones() const noexcept { return p (0) * p (2) / 100.0f + p (1) / 100.0f; }

    double fs = 48000.0;
    std::array<PitchShifterCore, 2> shifters;
    std::array<OnePoleLP, 2> tone;
    Smoother ratioSmooth;
};

//==============================================================================
/** Monophonic analog-style guitar synth: tracks the note with a zero-crossing
    detector, makes a square wave and a sub-octave, then sweeps a resonant
    filter with your picking. */
class GuitarSynth final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "synth", "Guitar Synth", "Pitch & Synth",
            "Turns single notes into a fat analog synth with sub-octave and an envelope-swept filter.",
            0xff5d2bd6, true,
            {
                ParamSpec::range ("sub", "Sub", 0.0f, 100.0f, 60.0f, "%", 0),
                ParamSpec::range ("square", "Square", 0.0f, 100.0f, 40.0f, "%", 0),
                ParamSpec::range ("dry", "Dry", 0.0f, 100.0f, 30.0f, "%", 0),
                ParamSpec::knob ("filter", "Filter", 4.0f),
                ParamSpec::knob ("resonance", "Resonance", 5.0f),
                ParamSpec::knob ("sweep", "Sweep", 6.0f),
            } };
        return info;
    }

    GuitarSynth() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& v : voices)
        {
            v.trackHp.set (70.0f, fs);
            v.track1.set (900.0f, fs);
            v.track2.set (900.0f, fs);
            v.antiAlias.set (7000.0f, fs);
            v.follower.prepare (fs, 2.0, 70.0);
        }
        reset();
    }

    void reset() override
    {
        for (auto& v : voices)
        {
            v.trackHp.reset(); v.track1.reset(); v.track2.reset(); v.antiAlias.reset();
            v.follower.reset(); v.filter.reset();
            v.square = 1.0f; v.sub = 1.0f;
        }
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const float subLevel = p (0) / 100.0f, squareLevel = p (1) / 100.0f, dry = p (2) / 100.0f;
        const float base = expMap (p (3) / 10.0f, 150.0f, 5000.0f);
        const float q = 0.7f + 0.75f * p (4);
        const float sweep = p (5) / 10.0f;

        for (int c = 0; c < numCh; ++c)
        {
            auto& v = voices[(size_t) c];
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                const float tracked = v.track2.process (v.track1.process (v.trackHp.process (x)));
                const float env = (float) v.follower.process (x);
                const float hysteresis = 0.15f * env + 1.0e-5f;

                if (v.square > 0.0f && tracked < -hysteresis)
                    v.square = -1.0f;
                else if (v.square < 0.0f && tracked > hysteresis)
                {
                    v.square = 1.0f;
                    v.sub = -v.sub; // flip-flop: half the frequency
                }

                const float gate = clamp01 ((env - 0.002f) * 400.0f);
                float synth = (v.sub * subLevel + v.square * squareLevel) * env * 1.0f * gate;
                synth = v.antiAlias.process (synth);

                if ((i & 7) == 0)
                {
                    const float envNorm = clamp01 (env * 8.0f);
                    v.filter.set (base * std::pow (2.0f, 4.0f * sweep * envNorm), q, fs);
                }

                ch[c][i] = softClip (v.filter.lowpass (synth)) + x * dry;
            }
        }
    }

private:
    struct Voice
    {
        OnePoleHP trackHp;
        OnePoleLP track1, track2, antiAlias;
        oap::dsp::EnvelopeFollower follower;
        Svf filter;
        float square = 1.0f, sub = 1.0f;
    };

    double fs = 48000.0;
    std::array<Voice, 2> voices;
};

//==============================================================================
/** Bit crushing, sample-rate reduction, tape wobble and band-limiting. */
class LoFi final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "lofi", "Lo-Fi", "Character",
            "Old samplers, worn tapes and cheap speakers: crush bits, drop the sample rate and add wobble.",
            0xffa0826d, true,
            {
                ParamSpec::range ("bits", "Bits", 4.0f, 16.0f, 10.0f, "", 0),
                ParamSpec::knob ("rate", "Downsample", 3.0f),
                ParamSpec::knob ("wobble", "Wobble", 3.0f),
                ParamSpec::knob ("filter", "Bandwidth", 6.0f),
                ParamSpec::knob ("drive", "Drive", 2.0f),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 100.0f, "%", 0),
            } };
        return info;
    }

    LoFi() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& l : lines) l.prepare ((int) (0.03 * fs));
        wow.setRate (0.7, fs);
        flutter.setRate (7.3, fs);
        reset();
    }

    void reset() override
    {
        for (auto& l : lines) l.reset();
        for (auto& f : lows) f.reset();
        for (auto& f : highs) f.reset();
        held = { 0.0f, 0.0f };
        holdPhase = 1.0;
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const float levels = std::pow (2.0f, std::round (p (0)) - 1.0f);
        const double targetRate = expMap (1.0f - p (1) / 10.0f, 1500.0f, (float) fs);
        const double holdIncrement = targetRate / fs;
        const float wobble = p (2) / 10.0f;
        const float bw = p (3) / 10.0f;
        for (auto& f : highs) f.set (expMap (1.0f - bw, 20.0f, 450.0f), fs);
        for (auto& f : lows) f.set (expMap (bw, 1800.0f, 19000.0f), fs);
        const float drive = 1.0f + p (4) * 0.8f;
        const float driveNorm = 1.0f / drive;
        float dryG, wetG;
        MixLaw::equalPower (p (5) / 100.0f, dryG, wetG);
        if (p (5) >= 99.5f) { dryG = 0.0f; wetG = 1.0f; }
        const float msToSamples = (float) (fs * 0.001);

        for (int i = 0; i < n; ++i)
        {
            const float delay = (6.0f + wobble * (3.5f * wow.sine() + 0.6f * flutter.sine())) * msToSamples;
            wow.advance();
            flutter.advance();

            holdPhase += holdIncrement;
            const bool sample = holdPhase >= 1.0;
            if (sample) holdPhase -= std::floor (holdPhase);

            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                lines[(size_t) c].push (x);
                float v = lines[(size_t) c].read (delay);
                v = softClip (v * drive) * driveNorm;
                v = lows[(size_t) c].process (highs[(size_t) c].process (v));
                if (sample)
                    held[(size_t) c] = std::round (v * levels) / levels;
                ch[c][i] = x * dryG + held[(size_t) c] * wetG;
            }
        }
    }

    int getLatencySamples() const noexcept override { return 0; }

private:
    double fs = 48000.0;
    std::array<DelayLine, 2> lines;
    std::array<OnePoleLP, 2> lows;
    std::array<OnePoleHP, 2> highs;
    std::array<float, 2> held {};
    double holdPhase = 1.0;
    Lfo wow, flutter;
};

} // namespace

void addFilterAndPitchEffects (std::vector<EffectEntry>& list)
{
    list.push_back ({ &Wah::describe(), &makeEffect<Wah> });
    list.push_back ({ &EnvelopeFilter::describe(), &makeEffect<EnvelopeFilter> });
    list.push_back ({ &PitchShifter::describe(), &makeEffect<PitchShifter> });
    list.push_back ({ &GuitarSynth::describe(), &makeEffect<GuitarSynth> });
    list.push_back ({ &LoFi::describe(), &makeEffect<LoFi> });
}

} // namespace oap::fx
