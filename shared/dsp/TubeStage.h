#pragma once

#include "Filters.h"

namespace oap::dsp
{

//==============================================================================
/** Padé [7/6] approximation of tanh, accurate to ~1e-6 inside |x| < 5. */
inline double fastTanh (double x) noexcept
{
    x = std::clamp (x, -5.0, 5.0);
    const double x2 = x * x;
    const double num = x * (135135.0 + x2 * (17325.0 + x2 * (378.0 + x2)));
    const double den = 135135.0 + x2 * (62370.0 + x2 * (3150.0 + 28.0 * x2));
    return std::clamp (num / den, -1.0, 1.0);
}

//==============================================================================
/** Biased tanh transfer curve that behaves like a triode gain stage.

    f(x) = (tanh(x + b) - tanh(b)) / (1 - tanh(b)^2)

    - Unity slope around zero, so the small-signal gain of a stage is exactly
      the gain applied before it.
    - A positive bias makes the positive half clip earlier and harder (grid
      conduction) while the negative half clips later and softer (cutoff),
      producing the even harmonics that make tube stages sound "warm".
    - The resulting DC shift is removed by the next stage's coupling filter.
*/
class TriodeCurve
{
public:
    void setBias (double newBias) noexcept
    {
        bias = newBias;
        tb = std::tanh (bias);
        norm = 1.0 / (1.0 - tb * tb);
    }

    double process (double x) const noexcept { return (fastTanh (x + bias) - tb) * norm; }

private:
    double bias = 0.0, tb = 0.0, norm = 1.0;
};

//==============================================================================
/** One preamp gain stage:

        coupling cap (HPF) -> gain -> Miller capacitance (LPF) -> triode curve

    The coupling high-pass sets how much low end reaches the clipping stage
    (the main "tightness" control of a high-gain amp); the low-pass limits how
    much high-frequency content gets distorted, keeping the fizz down.
*/
class TubeStage
{
public:
    struct Settings
    {
        double gain = 10.0;         // linear small-signal gain
        double bias = 0.2;          // asymmetry, 0 = symmetric
        double couplingHz = 20.0;   // input high-pass cutoff
        double bandwidthHz = 12000; // pre-clip low-pass cutoff
    };

    void prepare (double sampleRate, const Settings& s) noexcept
    {
        fs = sampleRate;
        settings = s;
        curve.setBias (s.bias);
        coupling.setCutoff (s.couplingHz, fs);
        miller.setCutoff (s.bandwidthHz, fs);
        reset();
    }

    void setCouplingHz (double hz) noexcept { coupling.setCutoff (hz, fs); }
    void setGain (double g) noexcept         { settings.gain = g; }

    void reset() noexcept
    {
        coupling.reset();
        miller.reset();
    }

    double process (double x) noexcept
    {
        x = coupling.highpass (x);
        x = miller.lowpass (x * settings.gain);
        return curve.process (x);
    }

private:
    double fs = 48000.0;
    Settings settings;
    TriodeCurve curve;
    OnePole coupling, miller;
};

//==============================================================================
/** Push-pull power amp: symmetric-ish saturation with supply sag.

    Sag lowers the headroom when the envelope of the signal is high, so
    loud passages compress and break up earlier, then recover - the "bloom"
    of a tube rectifier. Small signals pass with unity gain.
*/
class PowerAmp
{
public:
    void prepare (double sampleRate) noexcept
    {
        envelope.prepare (sampleRate, 4.0, 140.0);
        dcBlocker.setCutoff (7.0, sampleRate);
        curve.setBias (0.06);
        reset();
    }

    void setSag (double amount) noexcept { sag = std::max (0.0, amount); }

    void reset() noexcept
    {
        envelope.reset();
        dcBlocker.reset();
    }

    double process (double x) noexcept
    {
        const double env = std::min (envelope.process (x), 1.0); // caps sag at about -5 dB of headroom
        const double headroom = 1.0 / (1.0 + sag * env);
        const double y = headroom * curve.process (x / headroom);
        return dcBlocker.highpass (y);
    }

private:
    EnvelopeFollower envelope;
    OnePole dcBlocker;
    TriodeCurve curve;
    double sag = 0.3;
};

} // namespace oap::dsp
