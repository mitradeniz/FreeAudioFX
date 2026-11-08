#pragma once

// Small real-time DSP building blocks used by the effects (plain C++17).

#include "dsp/Filters.h"
#include "dsp/TubeStage.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace oap::fx
{

using oap::dsp::kPi;
constexpr double kTwoPi = 2.0 * kPi;

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float clamp01 (float x) noexcept { return std::clamp (x, 0.0f, 1.0f); }
inline float lerp (float a, float b, float t) noexcept { return a + t * (b - a); }

/** Maps 0..1 onto an exponential range, e.g. frequencies or times. */
inline float expMap (float t, float lo, float hi) noexcept { return lo * std::pow (hi / lo, clamp01 (t)); }

inline float softClip (float x) noexcept { return (float) oap::dsp::fastTanh (x); }

inline float flushDenormal (float x) noexcept { return std::abs (x) < 1.0e-15f ? 0.0f : x; }

//==============================================================================
/** One-pole parameter smoother (call next() once per sample). */
class Smoother
{
public:
    void prepare (double sampleRate, double timeMs) noexcept
    {
        coeff = (float) std::exp (-1.0 / (std::max (timeMs, 0.01) * 0.001 * sampleRate));
    }

    void reset (float value) noexcept { y = value; }
    float next (float target) noexcept { y = target + coeff * (y - target); return y; }
    float current() const noexcept { return y; }

private:
    float y = 0.0f, coeff = 0.0f;
};

//==============================================================================
/** Circular delay line with cubic (Hermite) interpolated reads. */
class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        size_t size = 16;
        while (size < (size_t) maxDelaySamples + 8)
            size <<= 1;
        buffer.assign (size, 0.0f);
        mask = size - 1;
        write = 0;
    }

    void reset() noexcept { std::fill (buffer.begin(), buffer.end(), 0.0f); write = 0; }

    void push (float x) noexcept
    {
        buffer[write] = x;
        write = (write + 1) & mask;
    }

    /** Sample written `delay` samples ago (delay >= 0, integer). */
    float tap (size_t delay) const noexcept { return buffer[(write - 1 - delay) & mask]; }

    /** Fractional read, `delay` in samples measured from the newest sample (clamped to >= 1). */
    float read (float delay) const noexcept
    {
        delay = std::clamp (delay, 1.0f, (float) (buffer.size() - 4));
        const float pos = (float) write - 1.0f - delay + (float) buffer.size();
        const auto i = (size_t) pos;
        const float t = pos - (float) i;

        const float ym1 = buffer[(i - 1) & mask];
        const float y0  = buffer[i & mask];
        const float y1  = buffer[(i + 1) & mask];
        const float y2  = buffer[(i + 2) & mask];

        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * t + c2) * t + c1) * t + y0;
    }

    size_t capacity() const noexcept { return buffer.size(); }

private:
    std::vector<float> buffer;
    size_t mask = 0, write = 0;
};

//==============================================================================
/** Phase accumulator LFO. */
class Lfo
{
public:
    void setRate (double hz, double sampleRate) noexcept { increment = hz / sampleRate; }
    void setPhase (double p) noexcept { phase = p - std::floor (p); }
    double getPhase() const noexcept { return phase; }

    void advance (int samples = 1) noexcept
    {
        phase += increment * samples;
        phase -= std::floor (phase);
    }

    float sine (double offset = 0.0) const noexcept { return (float) std::sin (kTwoPi * (phase + offset)); }

    float triangle (double offset = 0.0) const noexcept
    {
        double p = phase + offset;
        p -= std::floor (p);
        return (float) (p < 0.5 ? 4.0 * p - 1.0 : 3.0 - 4.0 * p);
    }

private:
    double phase = 0.0, increment = 0.0;
};

//==============================================================================
/** Topology-preserving state variable filter (Zavalishin / Simper). */
class Svf
{
public:
    void set (float cutoffHz, float q, double sampleRate) noexcept
    {
        cutoffHz = std::clamp (cutoffHz, 10.0f, (float) (0.45 * sampleRate));
        const float g = (float) std::tan (kPi * cutoffHz / sampleRate);
        k = 1.0f / std::max (q, 0.05f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void reset() noexcept { ic1 = ic2 = 0.0f; }

    void process (float x, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = flushDenormal (2.0f * v1 - ic1);
        ic2 = flushDenormal (2.0f * v2 - ic2);
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }

    float lowpass (float x) noexcept  { float l, b, h; process (x, l, b, h); return l; }
    float highpass (float x) noexcept { float l, b, h; process (x, l, b, h); return h; }
    float bandpass (float x) noexcept { float l, b, h; process (x, l, b, h); return b * k; } // unity peak

    float getK() const noexcept { return k; }

private:
    float k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f;
};

//==============================================================================
/** First-order all-pass section: H(z) = (a + z^-1) / (1 + a z^-1). */
class Allpass1
{
public:
    static float coefficientFor (float hz, double sampleRate) noexcept
    {
        const float t = (float) std::tan (kPi * std::clamp ((double) hz, 1.0, 0.45 * sampleRate) / sampleRate);
        return (t - 1.0f) / (t + 1.0f);
    }

    float process (float x, float a) noexcept
    {
        const float y = a * x + x1 - a * y1;
        x1 = x;
        y1 = flushDenormal (y);
        return y;
    }

    void reset() noexcept { x1 = y1 = 0.0f; }

private:
    float x1 = 0.0f, y1 = 0.0f;
};

//==============================================================================
/** Schroeder all-pass diffuser with a fixed delay. */
class Diffuser
{
public:
    void prepare (int delaySamples, float gainIn)
    {
        line.assign ((size_t) std::max (1, delaySamples), 0.0f);
        pos = 0;
        gain = gainIn;
    }

    void reset() noexcept { std::fill (line.begin(), line.end(), 0.0f); pos = 0; }

    float process (float x) noexcept
    {
        const float delayed = line[pos];
        const float v = x + gain * delayed;
        line[pos] = flushDenormal (v);
        pos = (pos + 1) % line.size();
        return delayed - gain * v;
    }

private:
    std::vector<float> line;
    size_t pos = 0;
    float gain = 0.5f;
};

//==============================================================================
/** Simple one-pole filters in float (for audio-rate use inside effects). */
struct OnePoleLP
{
    float a = 0.0f, y = 0.0f;
    void set (float hz, double sampleRate) noexcept { a = (float) std::exp (-kTwoPi * std::clamp ((double) hz, 1.0, 0.49 * sampleRate) / sampleRate); }
    float process (float x) noexcept { y = flushDenormal (x + a * (y - x)); return y; }
    void reset() noexcept { y = 0.0f; }
};

struct OnePoleHP
{
    OnePoleLP lp;
    void set (float hz, double sampleRate) noexcept { lp.set (hz, sampleRate); }
    float process (float x) noexcept { return x - lp.process (x); }
    void reset() noexcept { lp.reset(); }
};

//==============================================================================
/** Linear-phase FIR half-band oversampler (2x or 4x), one channel.
    The filters are designed at prepare() with a Kaiser window. Latency is an
    integer number of base-rate samples so it can be reported to the host. */
class Oversampler
{
public:
    void prepare (int factorIn, int maxBlockSize)
    {
        factor = factorIn == 4 ? 4 : 2;
        stage1 = design (63);  // base <-> 2x
        stage2 = design (33);  // 2x <-> 4x (33 taps keeps the total latency an integer)
        up1.assign (stage1.size(), 0.0f);  dn1.assign (stage1.size(), 0.0f);
        up2.assign (stage2.size(), 0.0f);  dn2.assign (stage2.size(), 0.0f);
        buffer.assign ((size_t) (maxBlockSize * factor + 8), 0.0f);
        reset();
    }

    void reset() noexcept
    {
        for (auto* v : { &up1, &dn1, &up2, &dn2 })
            std::fill (v->begin(), v->end(), 0.0f);
        p1u = p1d = p2u = p2d = 0;
    }

    int getFactor() const noexcept { return factor; }

    /** Base-rate latency of up + down sampling. */
    int getLatency() const noexcept
    {
        const int l1 = (int) (stage1.size() - 1); // (N-1)/2 per filter at 2x, two filters -> (N-1) samples at 2x
        const int l2 = (int) (stage2.size() - 1); // same at 4x
        return factor == 4 ? l1 / 2 + l2 / 4 : l1 / 2;
    }

    /** Upsamples n samples; returns a pointer to n * factor samples. */
    float* up (const float* in, int n) noexcept
    {
        int m = 0;
        for (int i = 0; i < n; ++i)
        {
            float a, b;
            upsample (stage1, up1, p1u, in[i], a, b);
            if (factor == 2)
            {
                buffer[(size_t) m++] = a;
                buffer[(size_t) m++] = b;
            }
            else
            {
                float c, d;
                upsample (stage2, up2, p2u, a, c, d);
                buffer[(size_t) m++] = c;
                buffer[(size_t) m++] = d;
                upsample (stage2, up2, p2u, b, c, d);
                buffer[(size_t) m++] = c;
                buffer[(size_t) m++] = d;
            }
        }
        return buffer.data();
    }

    /** Downsamples the internal buffer back into out (n base-rate samples). */
    void down (float* out, int n) noexcept
    {
        int m = 0;
        for (int i = 0; i < n; ++i)
        {
            float a, b;
            if (factor == 2)
            {
                a = buffer[(size_t) m++];
                b = buffer[(size_t) m++];
            }
            else
            {
                const float c = buffer[(size_t) m++], d = buffer[(size_t) m++];
                a = downsample (stage2, dn2, p2d, c, d);
                const float e = buffer[(size_t) m++], f = buffer[(size_t) m++];
                b = downsample (stage2, dn2, p2d, e, f);
            }
            out[i] = downsample (stage1, dn1, p1d, a, b);
        }
    }

private:
    static std::vector<float> design (int taps)
    {
        // Half-band low-pass at fs/4 of the higher rate, Kaiser window (beta 8).
        std::vector<float> h ((size_t) taps);
        const double beta = 8.0;
        const auto bessel0 = [] (double x)
        {
            double sum = 1.0, term = 1.0;
            for (int k = 1; k < 30; ++k)
            {
                term *= (x / (2.0 * k)) * (x / (2.0 * k));
                sum += term;
            }
            return sum;
        };
        const int mid = taps / 2;
        double total = 0.0;
        for (int i = 0; i < taps; ++i)
        {
            const int n = i - mid;
            const double sinc = n == 0 ? 0.5 : std::sin (kPi * n * 0.5) / (kPi * n);
            const double r = (double) n / mid;
            const double w = bessel0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / bessel0 (beta);
            h[(size_t) i] = (float) (sinc * w);
            total += h[(size_t) i];
        }
        for (auto& v : h)
            v = (float) (v / total);
        return h;
    }

    // Zero-stuffing upsampler producing two output samples (gain-compensated by 2).
    static void upsample (const std::vector<float>& h, std::vector<float>& state, size_t& pos, float x, float& o0, float& o1) noexcept
    {
        const size_t n = h.size();
        state[pos] = x;
        // Even/odd polyphase branches of the zero-stuffed signal.
        float s0 = 0.0f, s1 = 0.0f;
        size_t idx = pos;
        for (size_t k = 0; k < n; k += 2)
        {
            s0 += h[k] * state[idx];
            if (k + 1 < n)
                s1 += h[k + 1] * state[idx];
            idx = idx == 0 ? n - 1 : idx - 1;
        }
        pos = (pos + 1) % n;
        o0 = 2.0f * s0;
        o1 = 2.0f * s1;
    }

    // Filters two high-rate samples and returns one decimated sample (aligned
    // to the even sample, so the overall latency is a whole number of samples).
    static float downsample (const std::vector<float>& h, std::vector<float>& state, size_t& pos, float a, float b) noexcept
    {
        const size_t n = h.size();
        state[pos] = a;
        float s = 0.0f;
        size_t idx = pos;
        for (size_t k = 0; k < n; ++k)
        {
            s += h[k] * state[idx];
            idx = idx == 0 ? n - 1 : idx - 1;
        }
        pos = (pos + 1) % n;
        state[pos] = b;
        pos = (pos + 1) % n;
        return s;
    }

    int factor = 2;
    std::vector<float> stage1, stage2, up1, dn1, up2, dn2, buffer;
    size_t p1u = 0, p1d = 0, p2u = 0, p2d = 0;
};

} // namespace oap::fx
