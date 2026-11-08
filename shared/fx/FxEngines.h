#pragma once

// Larger DSP engines shared by several effects and the effect amps:
// FDN reverb, spring reverb, delay-line pitch shifter, random numbers.

#include "FxDsp.h"

#include <array>

namespace oap::fx
{

//==============================================================================
/** Tiny deterministic PRNG (xorshift32), safe to use on the audio thread. */
class Random
{
public:
    explicit Random (std::uint32_t seed = 0x9e3779b9u) : state (seed ? seed : 1u) {}

    std::uint32_t nextInt() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    /** Uniform in [0, 1). */
    float nextFloat() noexcept { return (float) (nextInt() >> 8) * (1.0f / 16777216.0f); }
    float nextBipolar() noexcept { return 2.0f * nextFloat() - 1.0f; }

private:
    std::uint32_t state;
};

//==============================================================================
/** Stereo feedback-delay-network reverb: 8 lines, Hadamard feedback matrix,
    input diffusion, per-line damping and gentle modulation. */
class FdnReverb
{
public:
    static constexpr int numLines = 8;

    void prepare (double sampleRate, float maxSize = 1.6f)
    {
        fs = sampleRate;
        const float scale = (float) (fs / 1000.0);
        for (int i = 0; i < numLines; ++i)
            lines[(size_t) i].prepare ((int) (baseLengthsMs[(size_t) i] * maxSize * scale) + 64);

        const float diffMs[4] = { 4.7f, 3.6f, 12.7f, 9.3f };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 4; ++i)
                diffusers[(size_t) c][(size_t) i].prepare ((int) (diffMs[i] * scale * (c == 0 ? 1.0f : 1.13f)),
                                                           i < 2 ? 0.7f : 0.6f);

        preDelay[0].prepare ((int) (0.25 * fs) + 8);
        preDelay[1].prepare ((int) (0.25 * fs) + 8);
        modLfo.setPhase (0.0);
        modLfo2.setPhase (0.37);
        modLfo.setRate (0.31, fs);
        modLfo2.setRate (0.47, fs);
        setParameters (2.0f, 1.0f, 6000.0f, 0.0f, 0.5f);
        reset();
    }

    void reset()
    {
        for (auto& l : lines) l.reset();
        for (auto& c : diffusers) for (auto& d : c) d.reset();
        for (auto& d : damping) d.reset();
        preDelay[0].reset();
        preDelay[1].reset();
        std::fill (feedback.begin(), feedback.end(), 0.0f);
    }

    /** decaySeconds = RT60, size 0.2..maxSize, dampingHz = high-frequency roll-off in the tail,
        preDelayMs, modDepth 0..1. */
    void setParameters (float decaySeconds, float size, float dampingHz, float preDelayMs, float modDepth)
    {
        const float scale = (float) (fs / 1000.0);
        for (int i = 0; i < numLines; ++i)
        {
            lengths[(size_t) i] = std::max (8.0f, baseLengthsMs[(size_t) i] * size * scale);
            gains[(size_t) i] = std::pow (10.0f, -3.0f * lengths[(size_t) i] / (std::max (0.1f, decaySeconds) * (float) fs));
            damping[(size_t) i].set (dampingHz, fs);
        }
        preDelaySamples = std::clamp (preDelayMs * scale, 1.0f, (float) (0.24 * fs));
        modSamples = modDepth * 0.0016f * (float) fs; // up to ~1.6 ms
    }

    void process (float inL, float inR, float& outL, float& outR) noexcept
    {
        preDelay[0].push (inL);
        preDelay[1].push (inR);
        float l = preDelay[0].read (preDelaySamples);
        float r = preDelay[1].read (preDelaySamples);
        for (auto& d : diffusers[0]) l = d.process (l);
        for (auto& d : diffusers[1]) r = d.process (r);

        std::array<float, numLines> v {};
        const float m1 = modSamples * modLfo.sine(), m2 = modSamples * modLfo2.sine();
        modLfo.advance();
        modLfo2.advance();

        for (int i = 0; i < numLines; ++i)
        {
            float len = lengths[(size_t) i];
            if (i == 1) len += m1;
            if (i == 6) len += m2;
            v[(size_t) i] = lines[(size_t) i].read (len);
        }

        outL = 0.5f * (v[0] - v[2] + v[4] - v[6] + v[1] + v[5]);
        outR = 0.5f * (v[1] - v[3] + v[5] - v[7] + v[2] + v[6]);

        // Fast Walsh-Hadamard transform (orthogonal mixing), normalised by 1/sqrt(8).
        for (int len = 1; len < numLines; len <<= 1)
            for (int i = 0; i < numLines; i += len << 1)
                for (int j = i; j < i + len; ++j)
                {
                    const float a = v[(size_t) j], b = v[(size_t) (j + len)];
                    v[(size_t) j] = a + b;
                    v[(size_t) (j + len)] = a - b;
                }

        constexpr float norm = 0.35355339f;
        for (int i = 0; i < numLines; ++i)
        {
            const float input = (i & 1) ? r : l;
            const float fb = damping[(size_t) i].process (v[(size_t) i] * norm * gains[(size_t) i]);
            lines[(size_t) i].push (flushDenormal (fb + input * 0.5f));
        }
    }

private:
    static constexpr std::array<float, numLines> baseLengthsMs { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.1f, 79.3f };

    double fs = 48000.0;
    std::array<DelayLine, numLines> lines;
    std::array<float, numLines> lengths {}, gains {}, feedback {};
    std::array<OnePoleLP, numLines> damping;
    std::array<std::array<Diffuser, 4>, 2> diffusers;
    DelayLine preDelay[2];
    float preDelaySamples = 1.0f, modSamples = 0.0f;
    Lfo modLfo, modLfo2;
};

//==============================================================================
/** Spring reverb: dispersive all-pass chains inside a feedback delay give the
    characteristic "boing" and drip of a spring tank. Two springs for stereo. */
class SpringReverb
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const int stretch = std::max (1, (int) std::lround (2.0 * fs / 48000.0));
        for (int s = 0; s < 2; ++s)
        {
            auto& spring = springs[(size_t) s];
            for (auto& stage : spring.chain)
                stage.assign ((size_t) stretch, 0.0f);
            spring.delay.prepare ((int) (0.08 * fs));
            spring.delaySamples = (float) ((s == 0 ? 0.0331 : 0.0397) * fs);
            spring.inputHp.set (280.0f, fs);
            spring.loopLp.set (4200.0f, fs);
            spring.outLp.set (5200.0f, fs);
        }
        modLfo.setRate (0.9, fs);
        setDecay (2.5f);
        reset();
    }

    void reset()
    {
        for (auto& spring : springs)
        {
            for (auto& stage : spring.chain)
                std::fill (stage.begin(), stage.end(), 0.0f);
            std::fill (spring.positions.begin(), spring.positions.end(), (size_t) 0);
            spring.delay.reset();
            spring.inputHp.reset();
            spring.loopLp.reset();
            spring.outLp.reset();
        }
    }

    void setDecay (float seconds)
    {
        for (auto& spring : springs)
            spring.loopGain = std::pow (10.0f, -3.0f * spring.delaySamples / (std::max (0.2f, seconds) * (float) fs));
    }

    void process (float inL, float inR, float& outL, float& outR) noexcept
    {
        const float mod = 1.5f * modLfo.sine();
        modLfo.advance();
        outL = processSpring (springs[0], inL, mod);
        outR = processSpring (springs[1], inR, -mod);
    }

private:
    static constexpr int numStages = 14;
    static constexpr float apCoeff = 0.62f;

    struct Spring
    {
        std::array<std::vector<float>, numStages> chain;
        std::array<size_t, numStages> positions {};
        DelayLine delay;
        float delaySamples = 1600.0f, loopGain = 0.5f;
        OnePoleHP inputHp;
        OnePoleLP loopLp, outLp;
    };

    float processSpring (Spring& s, float x, float mod) noexcept
    {
        float v = s.inputHp.process (x) + s.loopGain * s.delay.read (s.delaySamples + mod);

        // Stretched all-pass: H(z) = (a + z^-K) / (1 + a z^-K)
        for (int i = 0; i < numStages; ++i)
        {
            auto& buf = s.chain[(size_t) i];
            auto& pos = s.positions[(size_t) i];
            const float delayed = buf[pos];
            const float w = v - apCoeff * delayed;
            const float y = apCoeff * w + delayed;
            buf[pos] = flushDenormal (w);
            pos = (pos + 1) % buf.size();
            v = y;
        }

        s.delay.push (flushDenormal (softClip (s.loopLp.process (v))));
        return s.outLp.process (v);
    }

    double fs = 48000.0;
    std::array<Spring, 2> springs;
    Lfo modLfo;
};

//==============================================================================
/** Delay-line pitch shifter: two taps sweep through a short window and
    crossfade, so the pitch changes without changing the tempo. */
class PitchShifterCore
{
public:
    void prepare (double sampleRate, float windowMs = 50.0f)
    {
        window = (float) (windowMs * 0.001 * sampleRate);
        line.prepare ((int) window + 16);
        reset();
    }

    void reset()
    {
        line.reset();
        phase = 0.0;
    }

    float process (float x, float ratio) noexcept
    {
        line.push (x);
        phase += (1.0 - (double) ratio) / window;
        phase -= std::floor (phase);

        const float p1 = (float) phase;
        float p2 = p1 + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;

        const float g1 = std::sin ((float) kPi * p1);
        const float g2 = std::sin ((float) kPi * p2);
        return line.read (2.0f + p1 * window) * g1 * g1 + line.read (2.0f + p2 * window) * g2 * g2;
    }

    float getWindowSamples() const noexcept { return window; }

private:
    DelayLine line;
    float window = 2400.0f;
    double phase = 0.0;
};

//==============================================================================
/** Dry/wet laws. */
struct MixLaw
{
    /** "Pedal" law: dry stays at unity up to 50 %, then fades; wet reaches unity at 50 %. */
    static void pedal (float mix, float& dry, float& wet) noexcept
    {
        mix = clamp01 (mix);
        dry = mix <= 0.5f ? 1.0f : 2.0f * (1.0f - mix);
        wet = mix <= 0.5f ? 2.0f * mix : 1.0f;
    }

    /** Equal-power crossfade. */
    static void equalPower (float mix, float& dry, float& wet) noexcept
    {
        mix = clamp01 (mix);
        dry = std::cos (mix * (float) kPi * 0.5f);
        wet = std::sin (mix * (float) kPi * 0.5f);
    }
};

} // namespace oap::fx
