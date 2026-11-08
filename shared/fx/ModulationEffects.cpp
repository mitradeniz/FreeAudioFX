// Modulation pedals: Chorus, Flanger, Phaser, Tremolo, Rotary Speaker.

#include "FxEngines.h"
#include "Registry.h"

#include <array>

namespace oap::fx
{
namespace
{

//==============================================================================
class Chorus final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "chorus", "Chorus", "Modulation",
            "Lush analog-style chorus with vibrato and extra-wide stereo modes.",
            0xff4fb3d9, false,
            {
                ParamSpec::range ("rate", "Rate", 0.1f, 5.0f, 0.8f, "Hz", 2, 1.0f),
                ParamSpec::knob ("depth", "Depth", 5.0f),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 50.0f, "%", 0),
                ParamSpec::knob ("tone", "Tone", 6.0f),
                ParamSpec::choice ("mode", "Mode", { "Chorus", "Vibrato", "Wide" }, 0),
            } };
        return info;
    }

    Chorus() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& d : lines) d.prepare ((int) (0.05 * fs));
        depthSmooth.prepare (fs, 50.0);
        mixSmooth.prepare (fs, 30.0);
        reset();
    }

    void reset() override
    {
        for (auto& d : lines) d.reset();
        for (auto& t : tone) t.reset();
        lfo.setPhase (0.0);
        depthSmooth.reset (p (1) / 10.0f);
        mixSmooth.reset (p (2) / 100.0f);
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const int mode = choiceIndex (4);
        lfo.setRate (p (0), fs);
        for (auto& t : tone) t.set (expMap (p (3) / 10.0f, 1800.0f, 16000.0f), fs);

        const float msToSamples = (float) (fs * 0.001);
        const float base = (mode == 1 ? 2.0f : 6.5f) * msToSamples;
        const float range = (mode == 1 ? 3.0f : 6.0f) * msToSamples;
        const double rightOffset = mode == 2 ? 0.5 : 0.25;

        for (int i = 0; i < n; ++i)
        {
            const float depth = depthSmooth.next (p (1) / 10.0f);
            const float mix = mixSmooth.next (mode == 1 ? 1.0f : p (2) / 100.0f);
            float dryG, wetG;
            MixLaw::equalPower (mix, dryG, wetG);

            for (int c = 0; c < numCh; ++c)
            {
                const double off = c == 0 ? 0.0 : rightOffset;
                const float x = ch[c][i];
                lines[(size_t) c].push (x);

                float wet = lines[(size_t) c].read (base + range * depth * (0.5f + 0.5f * lfo.sine (off)));
                if (mode == 2) // second voice for the wide mode
                    wet = 0.5f * (wet + lines[(size_t) c].read (base * 1.3f + range * depth * (0.5f + 0.5f * lfo.triangle (off + 0.33))));

                wet = tone[(size_t) c].process (softClip (wet * 1.1f) / 1.1f);
                ch[c][i] = x * dryG + wet * wetG;
            }
            lfo.advance();
        }
    }

private:
    double fs = 48000.0;
    std::array<DelayLine, 2> lines;
    std::array<OnePoleLP, 2> tone;
    Lfo lfo;
    Smoother depthSmooth, mixSmooth;
};

//==============================================================================
class Flanger final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "flanger", "Flanger", "Modulation",
            "Jet-plane sweeps, from subtle shimmer to metallic regeneration.",
            0xff7b4fd1, false,
            {
                ParamSpec::range ("rate", "Rate", 0.05f, 5.0f, 0.3f, "Hz", 2, 0.6f),
                ParamSpec::knob ("depth", "Depth", 7.0f),
                ParamSpec::knob ("manual", "Manual", 5.0f),
                ParamSpec::range ("regen", "Regen", -95.0f, 95.0f, 50.0f, "%", 0),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 50.0f, "%", 0),
            } };
        return info;
    }

    Flanger() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& d : lines) d.prepare ((int) (0.02 * fs));
        manualSmooth.prepare (fs, 40.0);
        reset();
    }

    void reset() override
    {
        for (auto& d : lines) d.reset();
        fb = { 0.0f, 0.0f };
        lfo.setPhase (0.0);
        manualSmooth.reset (p (2));
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        lfo.setRate (p (0), fs);
        const float depth = 0.95f * p (1) / 10.0f;
        const float regen = 0.95f * p (3) / 100.0f;
        const float mix = p (4) / 100.0f;
        float dryG, wetG;
        MixLaw::equalPower (mix, dryG, wetG);

        for (int i = 0; i < n; ++i)
        {
            const float baseMs = expMap (manualSmooth.next (p (2)) / 10.0f, 0.3f, 6.0f);
            const float base = baseMs * (float) (fs * 0.001);
            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                const float mod = lfo.triangle (c == 0 ? 0.0 : 0.25);
                const float delay = std::max (2.0f, base * (1.0f + depth * mod));
                lines[(size_t) c].push (softClip (x + regen * fb[(size_t) c]));
                const float wet = lines[(size_t) c].read (delay);
                fb[(size_t) c] = flushDenormal (wet);
                ch[c][i] = x * dryG + wet * wetG;
            }
            lfo.advance();
        }
    }

private:
    double fs = 48000.0;
    std::array<DelayLine, 2> lines;
    std::array<float, 2> fb {};
    Lfo lfo;
    Smoother manualSmooth;
};

//==============================================================================
class Phaser final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "phaser", "Phaser", "Modulation",
            "Swirling all-pass phaser with 4 to 12 stages and resonant feedback.",
            0xfff08a24, false,
            {
                ParamSpec::range ("rate", "Rate", 0.05f, 8.0f, 0.5f, "Hz", 2, 1.0f),
                ParamSpec::knob ("depth", "Depth", 7.0f),
                ParamSpec::range ("feedback", "Feedback", 0.0f, 90.0f, 40.0f, "%", 0),
                ParamSpec::choice ("stages", "Stages", { "4", "6", "8", "12" }, 0),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 50.0f, "%", 0),
            } };
        return info;
    }

    Phaser() : Effect (describe()) {}

    void prepare (double sampleRate, int) override { fs = sampleRate; reset(); }

    void reset() override
    {
        for (auto& c : stages) for (auto& s : c) s.reset();
        last = { 0.0f, 0.0f };
        lfo.setPhase (0.0);
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        static constexpr int counts[] = { 4, 6, 8, 12 };
        const int numStages = counts[std::clamp (choiceIndex (3), 0, 3)];
        lfo.setRate (p (0), fs);
        const float fMax = expMap (p (1) / 10.0f, 300.0f, 3200.0f);
        const float fb = 0.9f * p (2) / 90.0f;
        const float mix = p (4) / 100.0f;

        std::array<float, 2> coeff {};
        for (int i = 0; i < n; ++i)
        {
            if ((i & 3) == 0)
                for (int c = 0; c < numCh; ++c)
                {
                    const float u = 0.5f + 0.5f * lfo.sine (c == 0 ? 0.0 : 0.25);
                    coeff[(size_t) c] = Allpass1::coefficientFor (expMap (u, 180.0f, fMax), fs);
                }

            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                float v = x + fb * last[(size_t) c];
                for (int s = 0; s < numStages; ++s)
                    v = stages[(size_t) c][(size_t) s].process (v, coeff[(size_t) c]);
                last[(size_t) c] = flushDenormal (softClip (v));
                ch[c][i] = x * (1.0f - mix) + v * mix;
            }
            lfo.advance();
        }
    }

private:
    double fs = 48000.0;
    std::array<std::array<Allpass1, 12>, 2> stages;
    std::array<float, 2> last {};
    Lfo lfo;
};

//==============================================================================
class Tremolo final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "tremolo", "Tremolo", "Modulation",
            "Vintage amp-style tremolo, harmonic tremolo and stereo auto-pan.",
            0xff1aa79c, false,
            {
                ParamSpec::range ("rate", "Rate", 0.5f, 15.0f, 5.0f, "Hz", 1, 4.0f),
                ParamSpec::range ("depth", "Depth", 0.0f, 100.0f, 60.0f, "%", 0),
                ParamSpec::knob ("shape", "Shape", 3.0f),
                ParamSpec::choice ("mode", "Mode", { "Classic", "Harmonic", "Pan" }, 0),
            } };
        return info;
    }

    Tremolo() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& s : gainSmooth) s.prepare (fs, 1.5);
        for (auto& x : crossover) x.set (750.0f, fs);
        reset();
    }

    void reset() override
    {
        lfo.setPhase (0.0);
        for (auto& s : gainSmooth) s.reset (1.0f);
        for (auto& x : crossover) x.reset();
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        lfo.setRate (p (0), fs);
        const float depth = p (1) / 100.0f;
        const float k = 0.6f + 0.8f * p (2);
        const float norm = 1.0f / std::tanh (k);
        const int mode = choiceIndex (3);

        for (int i = 0; i < n; ++i)
        {
            const float s = std::tanh (k * lfo.sine()) * norm; // -1..1, rounder -> squarer
            lfo.advance();
            const float down = 1.0f - depth * (0.5f - 0.5f * s);
            const float up = 1.0f - depth * (0.5f + 0.5f * s);

            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                if (mode == 1)
                {
                    const float low = crossover[(size_t) c].process (x);
                    const float high = x - low;
                    const float gl = gainSmooth[(size_t) (c * 2)].next (down);
                    const float gh = gainSmooth[(size_t) (c * 2 + 1)].next (up);
                    ch[c][i] = low * gl + high * gh;
                }
                else
                {
                    const float target = (mode == 2 && numCh == 2 && c == 1) ? up : down;
                    ch[c][i] = x * gainSmooth[(size_t) (c * 2)].next (target);
                }
            }
        }
    }

private:
    double fs = 48000.0;
    Lfo lfo;
    std::array<Smoother, 4> gainSmooth;
    std::array<OnePoleLP, 2> crossover;
};

//==============================================================================
/** Rotating speaker cabinet: separate horn and drum rotors with their own
    inertia, Doppler shift, amplitude modulation and two virtual microphones. */
class Rotary final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "rotary", "Rotary", "Modulation",
            "Rotating speaker cabinet with slow/fast speeds that spin up and down realistically.",
            0xff8a5a35, true,
            {
                ParamSpec::toggle ("fast", "Fast", false),
                ParamSpec::knob ("drive", "Drive", 3.0f),
                ParamSpec::knob ("balance", "Horn/Drum", 5.0f),
                ParamSpec::range ("spread", "Spread", 0.0f, 100.0f, 80.0f, "%", 0),
                ParamSpec::knob ("ramp", "Ramp", 5.0f),
            } };
        return info;
    }

    Rotary() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& d : hornLines) d.prepare ((int) (0.01 * fs));
        for (auto& d : drumLines) d.prepare ((int) (0.01 * fs));
        for (auto& f : lp) f.set (800.0f, 0.707f, fs);
        for (auto& f : hp) f.set (800.0f, 0.707f, fs);
        reset();
    }

    void reset() override
    {
        for (auto& d : hornLines) d.reset();
        for (auto& d : drumLines) d.reset();
        for (auto& f : lp) f.reset();
        for (auto& f : hp) f.reset();
        hornSpeed = toggled (0) ? 6.8 : 0.83;
        drumSpeed = toggled (0) ? 5.9 : 0.7;
        hornPhase = 0.0;
        drumPhase = 0.3;
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const bool fast = toggled (0);
        const double hornTarget = fast ? 6.8 : 0.83, drumTarget = fast ? 5.9 : 0.7;
        const double ramp = p (4) / 10.0;
        const double hornCoeff = 1.0 - std::exp (-1.0 / ((0.15 + ramp * 0.9) * fs));
        const double drumCoeff = 1.0 - std::exp (-1.0 / ((0.6 + ramp * 3.4) * fs));
        const float driveGain = 1.0f + p (1) * 0.6f;
        const float driveNorm = 1.0f / softClip (driveGain);
        const float bal = p (2) / 10.0f;
        const float hornGain = 0.33f + 0.42f * bal, drumGain = 0.75f - 0.42f * bal;
        const float micAngle = 0.5f * (float) kPi * p (3) / 100.0f;
        const float msToSamples = (float) (fs * 0.001);

        for (int i = 0; i < n; ++i)
        {
            float x = numCh == 2 ? 0.5f * (ch[0][i] + ch[1][i]) : ch[0][i];
            x = softClip (x * driveGain) * driveNorm;

            const float low = lp[1].lowpass (lp[0].lowpass (x));
            const float high = hp[1].highpass (hp[0].highpass (x));

            hornSpeed += (hornTarget - hornSpeed) * hornCoeff;
            drumSpeed += (drumTarget - drumSpeed) * drumCoeff;
            hornPhase += hornSpeed / fs; hornPhase -= std::floor (hornPhase);
            drumPhase += drumSpeed / fs; drumPhase -= std::floor (drumPhase);

            float out[2];
            for (int m = 0; m < 2; ++m)
            {
                const float angle = m == 0 ? -micAngle : micAngle;
                const float h = (float) (kTwoPi * hornPhase) + angle;
                const float d = (float) (kTwoPi * drumPhase) + angle;

                hornLines[(size_t) m].push (high);
                drumLines[(size_t) m].push (low);
                const float hornSig = hornLines[(size_t) m].read ((1.2f + 0.45f * std::sin (h)) * msToSamples)
                                      * (0.72f + 0.28f * std::cos (h));
                const float drumSig = drumLines[(size_t) m].read ((1.0f + 0.12f * std::sin (d)) * msToSamples)
                                      * (0.85f + 0.15f * std::cos (d));
                out[m] = hornSig * hornGain + drumSig * drumGain;
            }

            if (numCh == 2) { ch[0][i] = out[0]; ch[1][i] = out[1]; }
            else            { ch[0][i] = 0.5f * (out[0] + out[1]); }
        }
    }

private:
    double fs = 48000.0;
    std::array<DelayLine, 2> hornLines, drumLines;
    std::array<Svf, 2> lp, hp;
    double hornSpeed = 0.83, drumSpeed = 0.7, hornPhase = 0.0, drumPhase = 0.0;
};

} // namespace

void addModulationEffects (std::vector<EffectEntry>& list)
{
    list.push_back ({ &Chorus::describe(), &makeEffect<Chorus> });
    list.push_back ({ &Flanger::describe(), &makeEffect<Flanger> });
    list.push_back ({ &Phaser::describe(), &makeEffect<Phaser> });
    list.push_back ({ &Tremolo::describe(), &makeEffect<Tremolo> });
    list.push_back ({ &Rotary::describe(), &makeEffect<Rotary> });
}

} // namespace oap::fx
