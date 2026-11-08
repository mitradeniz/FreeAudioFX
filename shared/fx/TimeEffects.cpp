// Delay and reverb pedals: Delay, Reverb, Shimmer, Tape Echo, Freeze.

#include "FxEngines.h"
#include "Registry.h"

#include <array>

namespace oap::fx
{
namespace
{

//==============================================================================
class Delay final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "delay", "Delay", "Delay & Reverb",
            "Clean digital, warm analog or ping-pong echoes, free-running or synced to your song's tempo.",
            0xffe2ddd2, false,
            {
                ParamSpec::range ("time", "Time", 20.0f, 2000.0f, 380.0f, "ms", 0, 400.0f),
                ParamSpec::choice ("sync", "Sync", { "Free", "1/2", "1/4", "1/4 dot", "1/8", "1/8 dot", "1/8 trip", "1/16" }, 0),
                ParamSpec::range ("feedback", "Feedback", 0.0f, 100.0f, 35.0f, "%", 0),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 35.0f, "%", 0),
                ParamSpec::knob ("tone", "Tone", 6.0f),
                ParamSpec::choice ("mode", "Mode", { "Digital", "Analog", "Ping-Pong" }, 0),
            } };
        return info;
    }

    Delay() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& l : lines) l.prepare ((int) (2.6 * fs));
        timeSmooth.prepare (fs, 180.0);
        mixSmooth.prepare (fs, 30.0);
        modLfo.setRate (0.45, fs);
        for (auto& h : highpass) h.set (90.0f, fs);
        reset();
    }

    void reset() override
    {
        for (auto& l : lines) l.reset();
        for (auto& f : tone) f.reset();
        for (auto& h : highpass) h.reset();
        timeSmooth.reset (delaySamples());
        mixSmooth.reset (p (3) / 100.0f);
    }

    double getTailSeconds() const noexcept override { return 8.0; }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const int mode = choiceIndex (5);
        const float fb = std::min (0.98f, p (2) / 100.0f * (mode == 1 ? 1.05f : 1.0f));
        const float toneHz = mode == 1 ? expMap (p (4) / 10.0f, 1200.0f, 6500.0f) : expMap (p (4) / 10.0f, 2000.0f, 18000.0f);
        for (auto& f : tone) f.set (toneHz, fs);
        const float target = delaySamples();
        const float modDepth = mode == 1 ? 0.0004f * (float) fs : 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const float d = timeSmooth.next (target) + modDepth * modLfo.sine();
            modLfo.advance();
            float dryG, wetG;
            MixLaw::pedal (mixSmooth.next (p (3) / 100.0f), dryG, wetG);

            if (mode == 2) // ping-pong: mono input enters the left line, echoes alternate sides
            {
                const float in = numCh == 2 ? 0.5f * (ch[0][i] + ch[1][i]) : ch[0][i];
                const float l = lines[0].read (d), r = lines[1].read (d);
                lines[0].push (flushDenormal (in + fb * shape (1, r)));
                lines[1].push (flushDenormal (fb * shape (0, l)));
                if (numCh == 2)
                {
                    ch[0][i] = ch[0][i] * dryG + l * wetG;
                    ch[1][i] = ch[1][i] * dryG + r * wetG;
                }
                else
                {
                    ch[0][i] = in * dryG + 0.5f * (l + r) * wetG;
                }
                continue;
            }

            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                const float echo = lines[(size_t) c].read (d);
                float back = shape (c, echo);
                if (mode == 1)
                    back = softClip (back * 1.3f) / 1.3f;
                lines[(size_t) c].push (flushDenormal (x + fb * back));
                ch[c][i] = x * dryG + echo * wetG;
            }
        }
    }

private:
    float shape (int c, float x) noexcept { return tone[(size_t) c].process (highpass[(size_t) c].process (x)); }

    float delaySamples() const noexcept
    {
        static constexpr double beats[] = { 0.0, 2.0, 1.0, 1.5, 0.5, 0.75, 1.0 / 3.0, 0.25 };
        const int sync = std::clamp (choiceIndex (1), 0, 7);
        const double seconds = sync == 0 ? p (0) * 0.001 : beats[sync] * 60.0 / tempo;
        return (float) std::clamp (seconds * fs, 4.0, 2.5 * fs);
    }

    double fs = 48000.0;
    std::array<DelayLine, 2> lines;
    std::array<OnePoleLP, 2> tone;
    std::array<OnePoleHP, 2> highpass;
    Smoother timeSmooth, mixSmooth;
    Lfo modLfo;
};

//==============================================================================
class Reverb final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "reverb", "Reverb", "Delay & Reverb",
            "Room, hall, plate and spring reverbs in one pedal.",
            0xff2a3a6b, false,
            {
                ParamSpec::knob ("decay", "Decay", 5.0f),
                ParamSpec::range ("predelay", "Pre-delay", 0.0f, 200.0f, 15.0f, "ms", 0),
                ParamSpec::knob ("tone", "Tone", 6.0f),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 30.0f, "%", 0),
                ParamSpec::choice ("type", "Type", { "Room", "Hall", "Plate", "Spring" }, 1),
            } };
        return info;
    }

    Reverb() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        fdn.prepare (fs);
        spring.prepare (fs);
        mixSmooth.prepare (fs, 30.0);
        reset();
    }

    void reset() override
    {
        fdn.reset();
        spring.reset();
        mixSmooth.reset (p (3) / 100.0f);
        lastType = choiceIndex (4);
    }

    double getTailSeconds() const noexcept override { return 15.0; }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const int type = choiceIndex (4);
        if (type != lastType)
        {
            fdn.reset();
            spring.reset();
            lastType = type;
        }

        const float decayKnob = p (0) / 10.0f;
        const float toneHz = expMap (p (2) / 10.0f, 1500.0f, 14000.0f);
        switch (type)
        {
            case 0:  fdn.setParameters (expMap (decayKnob, 0.25f, 3.0f), 0.42f, toneHz, p (1), 0.3f); break;
            case 2:  fdn.setParameters (expMap (decayKnob, 0.5f, 8.0f), 0.65f, toneHz * 1.4f, p (1), 0.2f); break;
            case 3:  spring.setDecay (expMap (decayKnob, 0.6f, 5.0f)); break;
            default: fdn.setParameters (expMap (decayKnob, 0.8f, 14.0f), 1.25f, toneHz, p (1), 0.6f); break;
        }

        for (int i = 0; i < n; ++i)
        {
            float dryG, wetG;
            MixLaw::pedal (mixSmooth.next (p (3) / 100.0f), dryG, wetG);
            const float l = ch[0][i], r = numCh == 2 ? ch[1][i] : l;
            float wl, wr;
            if (type == 3)
                spring.process (l, r, wl, wr);
            else
                fdn.process (l, r, wl, wr);

            ch[0][i] = l * dryG + wl * wetG;
            if (numCh == 2)
                ch[1][i] = r * dryG + wr * wetG;
            else
                ch[0][i] = l * dryG + 0.5f * (wl + wr) * wetG;
        }
    }

private:
    double fs = 48000.0;
    FdnReverb fdn;
    SpringReverb spring;
    Smoother mixSmooth;
    int lastType = -1;
};

//==============================================================================
/** Long reverb with a pitch shifter in the feedback path, so the tail keeps
    climbing into octaves and fifths. */
class Shimmer final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "shimmer", "Shimmer", "Delay & Reverb",
            "Huge ambient reverb whose tail rises into octaves - instant cathedral pads.",
            0xffe58fb8, true,
            {
                ParamSpec::knob ("decay", "Decay", 7.0f),
                ParamSpec::range ("shimmer", "Shimmer", 0.0f, 100.0f, 50.0f, "%", 0),
                ParamSpec::choice ("interval", "Interval", { "+1 oct", "+5th", "+1 oct +5th", "+2 oct", "-1 oct" }, 0),
                ParamSpec::knob ("tone", "Tone", 6.0f),
                ParamSpec::range ("mix", "Mix", 0.0f, 100.0f, 40.0f, "%", 0),
            } };
        return info;
    }

    Shimmer() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        fdn.prepare (fs);
        shifter.prepare (fs, 70.0f);
        shimmerHp.set (250.0f, fs);
        mixSmooth.prepare (fs, 30.0);
        reset();
    }

    void reset() override
    {
        fdn.reset();
        shifter.reset();
        shimmerHp.reset();
        shimmerLp.reset();
        feedback = 0.0f;
        mixSmooth.reset (p (4) / 100.0f);
    }

    double getTailSeconds() const noexcept override { return 20.0; }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        static constexpr float semitones[] = { 12.0f, 7.0f, 19.0f, 24.0f, -12.0f };
        const float ratio = std::pow (2.0f, semitones[std::clamp (choiceIndex (2), 0, 4)] / 12.0f);
        const float toneHz = expMap (p (3) / 10.0f, 1500.0f, 12000.0f);
        fdn.setParameters (expMap (p (0) / 10.0f, 1.5f, 20.0f), 1.3f, toneHz, 30.0f, 0.7f);
        shimmerLp.set (toneHz, fs);
        const float amount = 0.55f * p (1) / 100.0f;

        for (int i = 0; i < n; ++i)
        {
            float dryG, wetG;
            MixLaw::pedal (mixSmooth.next (p (4) / 100.0f), dryG, wetG);
            const float l = ch[0][i], r = numCh == 2 ? ch[1][i] : l;
            const float shimmer = amount * feedback;
            float wl, wr;
            fdn.process (l + shimmer, r + shimmer, wl, wr);
            feedback = softClip (shimmerLp.process (shimmerHp.process (shifter.process (0.5f * (wl + wr), ratio))));

            if (numCh == 2)
            {
                ch[0][i] = l * dryG + wl * wetG;
                ch[1][i] = r * dryG + wr * wetG;
            }
            else
            {
                ch[0][i] = l * dryG + 0.5f * (wl + wr) * wetG;
            }
        }
    }

private:
    double fs = 48000.0;
    FdnReverb fdn;
    PitchShifterCore shifter;
    OnePoleHP shimmerHp;
    OnePoleLP shimmerLp;
    Smoother mixSmooth;
    float feedback = 0.0f;
};

//==============================================================================
/** Multi-head tape echo with wow, flutter, tape saturation and ageing. */
class TapeEcho final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "tapeecho", "Tape Echo", "Delay & Reverb",
            "Three playback heads, wobbly tape and saturated repeats that can run away into self-oscillation.",
            0xff5f7f3a, true,
            {
                ParamSpec::range ("rate", "Repeat Rate", 50.0f, 700.0f, 300.0f, "ms", 0, 250.0f),
                ParamSpec::knob ("intensity", "Intensity", 4.0f),
                ParamSpec::range ("echo", "Echo", 0.0f, 100.0f, 40.0f, "%", 0),
                ParamSpec::choice ("heads", "Heads", { "1", "2", "3", "1+2", "2+3", "1+2+3" }, 3),
                ParamSpec::knob ("wobble", "Wow & Flutter", 4.0f),
                ParamSpec::knob ("age", "Tape Age", 4.0f),
            } };
        return info;
    }

    TapeEcho() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        line.prepare ((int) (2.3 * fs));
        rateSmooth.prepare (fs, 250.0);
        mixSmooth.prepare (fs, 30.0);
        wow.setRate (0.55, fs);
        flutter.setRate (6.8, fs);
        loopHp.set (110.0f, fs);
        reset();
    }

    void reset() override
    {
        line.reset();
        loopHp.reset();
        loopLp.reset();
        outLp.reset();
        drift = 0.0f;
        rateSmooth.reset (p (0));
        mixSmooth.reset (p (2) / 100.0f);
    }

    double getTailSeconds() const noexcept override { return 10.0; }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        static constexpr std::array<std::array<float, 3>, 6> headMix {{
            {{ 1, 0, 0 }}, {{ 0, 1, 0 }}, {{ 0, 0, 1 }}, {{ 1, 1, 0 }}, {{ 0, 1, 1 }}, {{ 1, 1, 1 }} }};
        const auto& heads = headMix[(size_t) std::clamp (choiceIndex (3), 0, 5)];
        const float active = heads[0] + heads[1] + heads[2];
        const float fb = 1.1f * p (1) / 10.0f / std::sqrt (active);
        const float wobble = p (4) / 10.0f;
        const float age = p (5) / 10.0f;
        loopLp.set (expMap (1.0f - age, 2200.0f, 9000.0f), fs);
        outLp.set (expMap (1.0f - age, 3000.0f, 12000.0f), fs);

        for (int i = 0; i < n; ++i)
        {
            const float in = numCh == 2 ? 0.5f * (ch[0][i] + ch[1][i]) : ch[0][i];
            drift += 0.0005f * (random.nextBipolar() - drift * 0.02f);
            const float speedMod = 1.0f + wobble * (0.004f * wow.sine() + 0.0009f * flutter.sine() + 0.0015f * drift);
            wow.advance();
            flutter.advance();

            const float base = rateSmooth.next (p (0)) * 0.001f * (float) fs * speedMod;
            const float h1 = line.read (base), h2 = line.read (base * 2.0f), h3 = line.read (base * 3.0f);
            const float echo = (h1 * heads[0] + h2 * heads[1] + h3 * heads[2]) / std::sqrt (active);

            const float back = softClip (1.4f * loopLp.process (loopHp.process (echo))) / 1.4f;
            line.push (flushDenormal (softClip (in * 0.9f) + fb * back));

            float dryG, wetG;
            MixLaw::pedal (mixSmooth.next (p (2) / 100.0f), dryG, wetG);
            const float wet = outLp.process (echo);

            if (numCh == 2)
            {
                // head 1 leans left, head 3 leans right
                const float wl = wet + 0.25f * (h1 * heads[0] - h3 * heads[2]);
                const float wr = wet - 0.25f * (h1 * heads[0] - h3 * heads[2]);
                ch[0][i] = ch[0][i] * dryG + wl * wetG;
                ch[1][i] = ch[1][i] * dryG + wr * wetG;
            }
            else
            {
                ch[0][i] = in * dryG + wet * wetG;
            }
        }
    }

private:
    double fs = 48000.0;
    DelayLine line;
    Smoother rateSmooth, mixSmooth;
    Lfo wow, flutter;
    OnePoleHP loopHp;
    OnePoleLP loopLp, outLp;
    Random random { 0x51f15e5u };
    float drift = 0.0f;
};

//==============================================================================
/** Granular freeze: captures the last second of audio and turns it into an
    endless, evolving pad that you can play over. */
class Freeze final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "freeze", "Freeze", "Delay & Reverb",
            "Hit Freeze to hold whatever you just played as an endless pad, then play over it.",
            0xff8fd6e8, true,
            {
                ParamSpec::toggle ("freeze", "Freeze", false),
                ParamSpec::range ("level", "Level", 0.0f, 100.0f, 70.0f, "%", 0),
                ParamSpec::knob ("swell", "Swell", 3.0f),
                ParamSpec::knob ("texture", "Texture", 5.0f),
                ParamSpec::knob ("tone", "Tone", 7.0f),
            } };
        return info;
    }

    Freeze() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        history.assign ((size_t) (1.2 * fs), 0.0f);
        frozen.assign ((size_t) fs, 0.0f);
        for (auto& g : grains) g = {};
        reset();
    }

    void reset() override
    {
        std::fill (history.begin(), history.end(), 0.0f);
        std::fill (frozen.begin(), frozen.end(), 0.0f);
        writePos = 0;
        wasFrozen = false;
        envelope = 0.0f;
        for (auto& g : grains) g.active = false;
        for (auto& t : tone) t.reset();
    }

    double getTailSeconds() const noexcept override { return 3.0; }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const bool freeze = toggled (0);
        if (freeze && ! wasFrozen)
            capture();
        wasFrozen = freeze;

        const float level = p (1) / 100.0f;
        const float attack = 1.0f - std::exp (-1.0f / (expMap (p (2) / 10.0f, 0.03f, 3.0f) * (float) fs));
        const float release = 1.0f - std::exp (-1.0f / (1.5f * (float) fs));
        const float texture = p (3) / 10.0f;
        const int grainLength = (int) (expMap (texture, 0.06f, 0.4f) * fs);
        for (auto& t : tone) t.set (expMap (p (4) / 10.0f, 1200.0f, 16000.0f), fs);
        const int spacing = std::max (1, grainLength / (int) grains.size());

        for (int i = 0; i < n; ++i)
        {
            const float in = numCh == 2 ? 0.5f * (ch[0][i] + ch[1][i]) : ch[0][i];
            history[writePos] = in;
            writePos = (writePos + 1) % history.size();

            envelope += ((freeze ? 1.0f : 0.0f) - envelope) * (freeze ? attack : release);
            float wl = 0.0f, wr = 0.0f;

            if (envelope > 1.0e-4f)
            {
                if (++spawnCounter >= spacing)
                {
                    spawnCounter = 0;
                    spawn (grainLength, texture);
                }

                for (auto& g : grains)
                {
                    if (! g.active) continue;
                    const float t = (float) g.age / (float) g.length;
                    const float w = 0.5f - 0.5f * std::cos ((float) kTwoPi * t);
                    const double pos = g.start + g.age * g.rate;
                    const auto idx = (size_t) pos;
                    const float frac = (float) (pos - (double) idx);
                    const float s = frozen[idx % frozen.size()] * (1.0f - frac) + frozen[(idx + 1) % frozen.size()] * frac;
                    wl += s * w * (1.0f - g.pan);
                    wr += s * w * (1.0f + g.pan);
                    if (++g.age >= g.length) g.active = false;
                }
            }

            const float gain = envelope * level * 0.6f;
            wl = tone[0].process (wl) * gain;
            wr = tone[1].process (wr) * gain;

            if (numCh == 2) { ch[0][i] += wl; ch[1][i] += wr; }
            else            { ch[0][i] += 0.5f * (wl + wr); }
        }
    }

private:
    struct Grain
    {
        bool active = false;
        double start = 0.0, rate = 1.0;
        int age = 0, length = 1;
        float pan = 0.0f;
    };

    void capture() noexcept
    {
        // Copy the most recent second of input, oldest first.
        const size_t len = frozen.size();
        size_t read = (writePos + history.size() - len) % history.size();
        for (size_t i = 0; i < len; ++i)
        {
            frozen[i] = history[read];
            read = (read + 1) % history.size();
        }
    }

    void spawn (int length, float texture) noexcept
    {
        for (auto& g : grains)
        {
            if (g.active) continue;
            g.active = true;
            g.age = 0;
            g.length = length;
            const double maxStart = (double) frozen.size() - length * 1.02 - 2.0;
            g.start = std::max (0.0, maxStart * (0.15 + 0.85 * random.nextFloat()));
            g.rate = std::pow (2.0, (random.nextBipolar() * (2.0 + 10.0 * texture)) / 1200.0); // a few cents
            g.pan = 0.7f * random.nextBipolar();
            return;
        }
    }

    double fs = 48000.0;
    std::vector<float> history, frozen;
    size_t writePos = 0;
    bool wasFrozen = false;
    float envelope = 0.0f;
    int spawnCounter = 0;
    std::array<Grain, 8> grains;
    std::array<OnePoleLP, 2> tone;
    Random random { 0xf00du };
};

} // namespace

void addTimeEffects (std::vector<EffectEntry>& list)
{
    list.push_back ({ &Delay::describe(), &makeEffect<Delay> });
    list.push_back ({ &Reverb::describe(), &makeEffect<Reverb> });
    list.push_back ({ &Shimmer::describe(), &makeEffect<Shimmer> });
    list.push_back ({ &TapeEcho::describe(), &makeEffect<TapeEcho> });
    list.push_back ({ &Freeze::describe(), &makeEffect<Freeze> });
}

} // namespace oap::fx
