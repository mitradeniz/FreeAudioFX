#pragma once

#include "Filters.h"

namespace oap::dsp
{

//==============================================================================
/** Passive treble/middle/bass tone stack (the classic "TMB" circuit used in
    most Fender and Marshall style amps).

    Implements the closed-form analog transfer function derived by
    D. T. Yeh and J. O. Smith, "Discretization of the '59 Fender Bassman Tone
    Stack" (DAFx-06), discretised with the bilinear transform. Because it is a
    model of the real passive network, the controls interact the way they do
    on a real amp and the response never exceeds 0 dB.
*/
class ToneStack
{
public:
    struct Components
    {
        double R1, R2, R3, R4; // treble pot, bass pot, mid pot, slope resistor (ohms)
        double C1, C2, C3;     // treble, bass, mid capacitors (farads)
    };

    /** '59 Bassman style values: wide, scooped, warm. */
    static constexpr Components bassman()  { return { 250e3, 1e6, 25e3, 56e3, 250e-12, 20e-9, 20e-9 }; }
    /** British high-gain values: more mids, tighter lows. */
    static constexpr Components british()  { return { 220e3, 1e6, 22e3, 33e3, 470e-12, 22e-9, 22e-9 }; }
    /** Blackface clean values: big bass cap, deep mid scoop. */
    static constexpr Components blackface() { return { 250e3, 250e3, 10e3, 100e3, 120e-12, 100e-9, 47e-9 }; }

    void prepare (double sampleRate, const Components& c) noexcept
    {
        fs = sampleRate;
        comp = c;
        reset();
        setControls (0.5, 0.5, 0.5);
    }

    void reset() noexcept { s1 = s2 = s3 = 0.0; }

    /** Knob positions 0..1. Bass and middle use an audio (log) taper like the real pots. */
    void setControls (double bass, double middle, double treble) noexcept
    {
        const double l = taper (bass);
        const double m = taper (middle);
        const double t = std::clamp (treble, 0.0, 1.0);
        computeAnalog (l, m, t);
        bilinear();
    }

    double process (double x) noexcept
    {
        const double y = B[0] * x + s1;
        s1 = B[1] * x - A[1] * y + s2;
        s2 = B[2] * x - A[2] * y + s3;
        s3 = B[3] * x - A[3] * y;
        return y;
    }

    /** Magnitude of the discretised filter at a frequency (for tests/visualisation). */
    double magnitudeAt (double hz) const noexcept
    {
        const std::complex<double> z1 = std::polar (1.0, -2.0 * kPi * hz / fs);
        const auto z2 = z1 * z1, z3 = z2 * z1;
        return std::abs ((B[0] + B[1] * z1 + B[2] * z2 + B[3] * z3)
                         / (1.0 + A[1] * z1 + A[2] * z2 + A[3] * z3));
    }

    static double taper (double knob) noexcept
    {
        // audio taper: 0 -> ~0.03, 0.5 -> ~0.18, 1 -> 1
        return std::exp ((std::clamp (knob, 0.0, 1.0) - 1.0) * 3.4);
    }

private:
    void computeAnalog (double l, double m, double t) noexcept
    {
        const auto& [R1, R2, R3, R4, C1, C2, C3] = comp;

        b1 = t * C1 * R1 + m * C3 * R3 + l * (C1 * R2 + C2 * R2) + (C1 * R3 + C2 * R3);

        b2 = t * (C1 * C2 * R1 * R4 + C1 * C3 * R1 * R4)
           - m * m * (C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
           + m * (C1 * C3 * R1 * R3 + C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
           + l * (C1 * C2 * R1 * R2 + C1 * C2 * R2 * R4 + C1 * C3 * R2 * R4)
           + l * m * (C1 * C3 * R2 * R3 + C2 * C3 * R2 * R3)
           + (C1 * C2 * R1 * R3 + C1 * C2 * R3 * R4 + C1 * C3 * R3 * R4);

        b3 = l * m * (C1 * C2 * C3 * R1 * R2 * R3 + C1 * C2 * C3 * R2 * R3 * R4)
           - m * m * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4)
           + m * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4)
           + t * C1 * C2 * C3 * R1 * R3 * R4
           - t * m * C1 * C2 * C3 * R1 * R3 * R4
           + t * l * C1 * C2 * C3 * R1 * R2 * R4;

        a0 = 1.0;

        a1 = (C1 * R1 + C1 * R3 + C2 * R3 + C2 * R4 + C3 * R4)
           + m * C3 * R3 + l * (C1 * R2 + C2 * R2);

        a2 = m * (C1 * C3 * R1 * R3 - C2 * C3 * R3 * R4 + C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
           + l * m * (C1 * C3 * R2 * R3 + C2 * C3 * R2 * R3)
           - m * m * (C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
           + l * (C1 * C2 * R2 * R4 + C1 * C2 * R1 * R2 + C1 * C3 * R2 * R4 + C2 * C3 * R2 * R4)
           + (C1 * C2 * R1 * R4 + C1 * C3 * R1 * R4 + C1 * C2 * R3 * R4
              + C1 * C2 * R1 * R3 + C1 * C3 * R3 * R4 + C2 * C3 * R3 * R4);

        a3 = l * m * (C1 * C2 * C3 * R1 * R2 * R3 + C1 * C2 * C3 * R2 * R3 * R4)
           - m * m * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4)
           + m * (C1 * C2 * C3 * R3 * R3 * R4 + C1 * C2 * C3 * R1 * R3 * R3
                  - C1 * C2 * C3 * R1 * R3 * R4)
           + l * C1 * C2 * C3 * R1 * R2 * R4
           + C1 * C2 * C3 * R1 * R3 * R4;
    }

    /** s = c (1 - z^-1) / (1 + z^-1), c = 2 fs. */
    void bilinear() noexcept
    {
        const double c = 2.0 * fs, c2 = c * c, c3 = c2 * c;

        const double nb0 =  b1 * c + b2 * c2 + b3 * c3;
        const double nb1 =  b1 * c - b2 * c2 - 3.0 * b3 * c3;
        const double nb2 = -b1 * c - b2 * c2 + 3.0 * b3 * c3;
        const double nb3 = -b1 * c + b2 * c2 - b3 * c3;

        const double na0 =       a0 + a1 * c + a2 * c2 + a3 * c3;
        const double na1 = 3.0 * a0 + a1 * c - a2 * c2 - 3.0 * a3 * c3;
        const double na2 = 3.0 * a0 - a1 * c - a2 * c2 + 3.0 * a3 * c3;
        const double na3 =       a0 - a1 * c + a2 * c2 - a3 * c3;

        const double inv = 1.0 / na0;
        B[0] = nb0 * inv; B[1] = nb1 * inv; B[2] = nb2 * inv; B[3] = nb3 * inv;
        A[0] = 1.0;       A[1] = na1 * inv; A[2] = na2 * inv; A[3] = na3 * inv;
    }

    double fs = 48000.0;
    Components comp = bassman();
    double b1 = 0, b2 = 0, b3 = 0, a0 = 1, a1 = 0, a2 = 0, a3 = 0;
    double B[4] {}, A[4] { 1.0, 0.0, 0.0, 0.0 };
    double s1 = 0.0, s2 = 0.0, s3 = 0.0;
};

} // namespace oap::dsp
