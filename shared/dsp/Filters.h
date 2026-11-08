#pragma once

#include <algorithm>
#include <cmath>
#include <complex>

namespace oap::dsp
{

constexpr double kPi = 3.14159265358979323846;

/** True when a control value has moved enough to be worth recomputing coefficients. */
inline bool hasChanged (float a, float b) noexcept { return std::abs (a - b) > 1.0e-6f; }

//==============================================================================
/** First-order filter in topology-preserving-transform (TPT) form.

    Stays well behaved when the cutoff is modulated and has accurate low
    cutoffs at high (oversampled) rates. Use one instance either as a lowpass
    or as a highpass, not both.
*/
class OnePole
{
public:
    void setCutoff (double hz, double sampleRate) noexcept
    {
        hz = std::clamp (hz, 1.0, 0.45 * sampleRate);
        const double g = std::tan (kPi * hz / sampleRate);
        G = g / (1.0 + g);
    }

    void reset (double value = 0.0) noexcept { s = value; }

    double lowpass (double x) noexcept
    {
        const double v = (x - s) * G;
        const double y = v + s;
        s = y + v;
        return y;
    }

    double highpass (double x) noexcept { return x - lowpass (x); }

private:
    double G = 0.0, s = 0.0;
};

//==============================================================================
/** Normalised biquad coefficients (a0 == 1), designed with the RBJ cookbook. */
struct BiquadCoeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

    static BiquadCoeffs identity() noexcept { return {}; }

    static BiquadCoeffs lowpass (double fs, double hz, double q) noexcept
    {
        const auto p = prewarp (fs, hz, q);
        const double b = (1.0 - p.cosw) * 0.5;
        return normalise (b, 1.0 - p.cosw, b, 1.0 + p.alpha, -2.0 * p.cosw, 1.0 - p.alpha);
    }

    static BiquadCoeffs highpass (double fs, double hz, double q) noexcept
    {
        const auto p = prewarp (fs, hz, q);
        const double b = (1.0 + p.cosw) * 0.5;
        return normalise (b, -(1.0 + p.cosw), b, 1.0 + p.alpha, -2.0 * p.cosw, 1.0 - p.alpha);
    }

    static BiquadCoeffs peak (double fs, double hz, double q, double gainDb) noexcept
    {
        const auto p = prewarp (fs, hz, q);
        const double A = std::pow (10.0, gainDb / 40.0);
        return normalise (1.0 + p.alpha * A, -2.0 * p.cosw, 1.0 - p.alpha * A,
                          1.0 + p.alpha / A, -2.0 * p.cosw, 1.0 - p.alpha / A);
    }

    static BiquadCoeffs lowShelf (double fs, double hz, double q, double gainDb) noexcept
    {
        const auto p = prewarp (fs, hz, q);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double k = 2.0 * std::sqrt (A) * p.alpha;
        return normalise (A * ((A + 1.0) - (A - 1.0) * p.cosw + k),
                          2.0 * A * ((A - 1.0) - (A + 1.0) * p.cosw),
                          A * ((A + 1.0) - (A - 1.0) * p.cosw - k),
                          (A + 1.0) + (A - 1.0) * p.cosw + k,
                          -2.0 * ((A - 1.0) + (A + 1.0) * p.cosw),
                          (A + 1.0) + (A - 1.0) * p.cosw - k);
    }

    static BiquadCoeffs highShelf (double fs, double hz, double q, double gainDb) noexcept
    {
        const auto p = prewarp (fs, hz, q);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double k = 2.0 * std::sqrt (A) * p.alpha;
        return normalise (A * ((A + 1.0) + (A - 1.0) * p.cosw + k),
                          -2.0 * A * ((A - 1.0) + (A + 1.0) * p.cosw),
                          A * ((A + 1.0) + (A - 1.0) * p.cosw - k),
                          (A + 1.0) - (A - 1.0) * p.cosw + k,
                          2.0 * ((A - 1.0) - (A + 1.0) * p.cosw),
                          (A + 1.0) - (A - 1.0) * p.cosw - k);
    }

    /** Magnitude response at a given frequency (useful for tests and UI). */
    double magnitudeAt (double hz, double fs) const noexcept
    {
        const std::complex<double> z1 = std::polar (1.0, -2.0 * kPi * hz / fs);
        const std::complex<double> z2 = z1 * z1;
        return std::abs ((b0 + b1 * z1 + b2 * z2) / (1.0 + a1 * z1 + a2 * z2));
    }

private:
    struct Prewarped { double cosw, alpha; };

    static Prewarped prewarp (double fs, double hz, double q) noexcept
    {
        hz = std::clamp (hz, 1.0, 0.49 * fs);
        const double w0 = 2.0 * kPi * hz / fs;
        return { std::cos (w0), std::sin (w0) / (2.0 * std::max (q, 1.0e-3)) };
    }

    static BiquadCoeffs normalise (double b0, double b1, double b2,
                                   double a0, double a1, double a2) noexcept
    {
        const double inv = 1.0 / a0;
        return { b0 * inv, b1 * inv, b2 * inv, a1 * inv, a2 * inv };
    }
};

//==============================================================================
/** Biquad in transposed direct form II, double precision. */
class Biquad
{
public:
    void setCoeffs (const BiquadCoeffs& c) noexcept { k = c; }
    const BiquadCoeffs& getCoeffs() const noexcept { return k; }
    void reset() noexcept { z1 = z2 = 0.0; }

    double process (double x) noexcept
    {
        const double y = k.b0 * x + z1;
        z1 = k.b1 * x - k.a1 * y + z2;
        z2 = k.b2 * x - k.a2 * y;
        return y;
    }

private:
    BiquadCoeffs k;
    double z1 = 0.0, z2 = 0.0;
};

//==============================================================================
/** Smooth one-pole envelope follower with separate attack and release. */
class EnvelopeFollower
{
public:
    void prepare (double sampleRate, double attackMs, double releaseMs) noexcept
    {
        att = coeff (sampleRate, attackMs);
        rel = coeff (sampleRate, releaseMs);
    }

    void reset() noexcept { env = 0.0; }

    double process (double input) noexcept
    {
        const double a = std::abs (input);
        env += (a > env ? att : rel) * (a - env);
        return env;
    }

    double getValue() const noexcept { return env; }

private:
    static double coeff (double fs, double ms) noexcept
    {
        return 1.0 - std::exp (-1.0 / (std::max (ms, 0.01) * 0.001 * fs));
    }

    double att = 1.0, rel = 1.0, env = 0.0;
};

} // namespace oap::dsp
