#pragma once

#include "Filters.h"

#include <array>
#include <vector>

namespace oap::dsp
{

//==============================================================================
/** Filter-based loudspeaker cabinet voicings.

    These are hand-tuned approximations of typical guitar cabinet responses
    (low resonance, box dip, speaker presence peak, steep cone roll-off), not
    captures of any specific commercial product. They are cheap, latency-free
    and work at any sample rate. For a specific cab, load an impulse response.
*/
class CabinetModel
{
public:
    enum class Voicing { Closed4x12, Open2x12, Combo1x12 };

    void prepare (double sampleRate, Voicing v)
    {
        fs = sampleRate;
        voicing = v;
        numSections = 0;

        switch (v)
        {
            case Voicing::Closed4x12:
                add (BiquadCoeffs::highpass (fs, 72.0, 0.85));
                add (BiquadCoeffs::peak (fs, 110.0, 1.3, 3.5));    // cabinet thump
                add (BiquadCoeffs::peak (fs, 420.0, 1.1, -3.0));   // box dip
                add (BiquadCoeffs::peak (fs, 2300.0, 1.3, 4.0));   // cone presence
                add (BiquadCoeffs::peak (fs, 3800.0, 2.0, 2.0));
                addButterworthLowpass4 (5400.0);
                add (BiquadCoeffs::lowpass (fs, 9000.0, 0.707));
                break;

            case Voicing::Open2x12:
                add (BiquadCoeffs::highpass (fs, 60.0, 0.65));
                add (BiquadCoeffs::peak (fs, 95.0, 1.0, 1.5));
                add (BiquadCoeffs::peak (fs, 650.0, 0.9, -2.0));
                add (BiquadCoeffs::peak (fs, 1900.0, 1.1, 3.0));
                add (BiquadCoeffs::peak (fs, 3400.0, 1.8, 2.5));
                addButterworthLowpass4 (6200.0);
                add (BiquadCoeffs::lowpass (fs, 10500.0, 0.707));
                break;

            case Voicing::Combo1x12:
                add (BiquadCoeffs::highpass (fs, 90.0, 1.0));
                add (BiquadCoeffs::peak (fs, 140.0, 1.2, 2.0));
                add (BiquadCoeffs::peak (fs, 900.0, 0.9, 1.5));
                add (BiquadCoeffs::peak (fs, 2800.0, 1.5, 3.0));
                addButterworthLowpass4 (5000.0);
                add (BiquadCoeffs::lowpass (fs, 8500.0, 0.707));
                break;
        }

        // Normalise so every voicing has a similar perceived level.
        double sum = 0.0;
        const auto freqs = referenceFrequencies();
        for (double f : freqs)
        {
            double mag = 1.0;
            for (int i = 0; i < numSections; ++i)
                mag *= sections[(size_t) i].getCoeffs().magnitudeAt (f, fs);
            sum += mag * mag;
        }
        trim = 1.0 / std::sqrt (sum / (double) freqs.size());

        reset();
    }

    void reset() noexcept
    {
        for (auto& s : sections)
            s.reset();
    }

    double process (double x) noexcept
    {
        for (int i = 0; i < numSections; ++i)
            x = sections[(size_t) i].process (x);
        return x * trim;
    }

    double magnitudeAt (double hz) const noexcept
    {
        double mag = trim;
        for (int i = 0; i < numSections; ++i)
            mag *= sections[(size_t) i].getCoeffs().magnitudeAt (hz, fs);
        return mag;
    }

    /** Log-spaced frequencies used to level-match voicings and impulse responses. */
    static std::vector<double> referenceFrequencies()
    {
        std::vector<double> f;
        constexpr int n = 48;
        for (int i = 0; i < n; ++i)
            f.push_back (80.0 * std::pow (6000.0 / 80.0, (double) i / (n - 1)));
        return f;
    }

private:
    void add (const BiquadCoeffs& c) noexcept
    {
        if (numSections < (int) sections.size())
            sections[(size_t) numSections++].setCoeffs (c);
    }

    void addButterworthLowpass4 (double hz) noexcept
    {
        add (BiquadCoeffs::lowpass (fs, hz, 0.5412));
        add (BiquadCoeffs::lowpass (fs, hz, 1.3066));
    }

    double fs = 48000.0;
    Voicing voicing = Voicing::Closed4x12;
    std::array<Biquad, 10> sections;
    int numSections = 0;
    double trim = 1.0;
};

} // namespace oap::dsp
