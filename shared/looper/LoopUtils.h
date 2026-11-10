#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace oap::looper
{

//==============================================================================
/** Length of a loop after converting it to another sample rate. */
inline int resampledLength (int numSamples, double fromRate, double toRate, int maxSamples) noexcept
{
    if (numSamples <= 0 || fromRate <= 0.0 || toRate <= 0.0)
        return 0;

    const double n = std::round ((double) numSamples * toRate / fromRate);
    return (int) std::clamp (n, 1.0, (double) std::max (1, maxSamples));
}

/** Converts one channel of a loop from `numIn` to `numOut` samples (windowed
    sinc). The audio is treated as circular: what comes after the last sample
    is the first one, so a seamless loop stays seamless and nothing is shifted
    in time. Not for the audio thread. */
inline void resampleLoop (const float* in, int numIn, float* out, int numOut)
{
    if (numIn <= 0 || numOut <= 0)
        return;

    if (numIn == numOut)
    {
        std::memcpy (out, in, (size_t) numIn * sizeof (float));
        return;
    }

    const double pi = 3.14159265358979323846;
    const double ratio = (double) numIn / (double) numOut;       // input samples per output sample
    const double cutoff = 0.94 * std::min (1.0, 1.0 / ratio);    // relative to the input's Nyquist frequency
    const int half = (int) std::ceil (32.0 / std::min (1.0, 1.0 / ratio)); // 32 zero crossings on each side
    const int taps = 2 * half;
    const int phases = 512;

    // Kaiser window (beta 8: about -80 dB)
    const auto bessel = [] (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 40; ++k)
        {
            term *= (x / (2.0 * k)) * (x / (2.0 * k));
            sum += term;
        }
        return sum;
    };
    const double beta = 8.0, besselBeta = bessel (beta);

    // One filter per fractional position (plus one more to interpolate towards).
    std::vector<float> table ((size_t) (phases + 1) * (size_t) taps);
    for (int p = 0; p <= phases; ++p)
    {
        const double frac = (double) p / (double) phases;
        double sum = 0.0;
        float* row = table.data() + (size_t) p * (size_t) taps;

        for (int j = 0; j < taps; ++j)
        {
            const double x = (double) (j - half + 1) - frac; // distance from the output position, in input samples
            const double w = x / (double) half;
            const double window = std::abs (w) < 1.0 ? bessel (beta * std::sqrt (1.0 - w * w)) / besselBeta : 0.0;
            const double arg = pi * cutoff * x;
            const double sinc = std::abs (arg) < 1.0e-9 ? 1.0 : std::sin (arg) / arg;
            const double h = cutoff * sinc * window;
            row[j] = (float) h;
            sum += h;
        }

        for (int j = 0; j < taps; ++j)
            row[j] = (float) (row[j] / sum); // unity gain at every position
    }

    // The input with its ends wrapped around, so the filter never needs a modulo.
    std::vector<float> wrapped ((size_t) numIn + (size_t) taps);
    for (int j = 0; j < numIn + taps; ++j)
    {
        int index = (j - half + 1) % numIn;
        if (index < 0)
            index += numIn;
        wrapped[(size_t) j] = in[index];
    }

    for (int i = 0; i < numOut; ++i)
    {
        const double position = (double) i * ratio;
        int base = (int) position;
        double phase = (position - (double) base) * (double) phases;
        int p = (int) phase;
        if (p >= phases)
            p = phases - 1;
        const float a = (float) (phase - (double) p);
        if (base >= numIn)
            base = numIn - 1;

        const float* row0 = table.data() + (size_t) p * (size_t) taps;
        const float* row1 = row0 + taps;
        const float* x = wrapped.data() + base;

        float sum = 0.0f;
        for (int j = 0; j < taps; ++j)
            sum += x[j] * (row0[j] + a * (row1[j] - row0[j]));
        out[i] = sum;
    }
}

/** Converts a stereo loop to another sample rate (see resampleLoop). */
inline void resampleLoop (std::vector<float>& left, std::vector<float>& right, double fromRate, double toRate, int maxSamples)
{
    const int numIn = (int) std::min (left.size(), right.size());
    const int numOut = resampledLength (numIn, fromRate, toRate, maxSamples);

    if (numIn <= 0 || numOut <= 0)
    {
        left.clear();
        right.clear();
        return;
    }

    if (numOut == numIn)
    {
        left.resize ((size_t) numIn);
        right.resize ((size_t) numIn);
        return;
    }

    std::vector<float> result ((size_t) numOut);
    resampleLoop (left.data(), numIn, result.data(), numOut);
    left.swap (result);
    result.assign ((size_t) numOut, 0.0f);
    resampleLoop (right.data(), numIn, result.data(), numOut);
    right.swap (result);
}

//==============================================================================
/** A coarse picture of a loop for the user interface: the peak level of
    `numBins` equal slices. */
inline void makeOverview (const float* left, const float* right, int numSamples, int numBins, std::vector<float>& peaks)
{
    peaks.assign ((size_t) std::max (0, numBins), 0.0f);
    if (numSamples <= 0 || numBins <= 0)
        return;

    for (int bin = 0; bin < numBins; ++bin)
    {
        const auto start = (int) ((std::int64_t) bin * numSamples / numBins);
        const auto end = std::max (start + 1, (int) ((std::int64_t) (bin + 1) * numSamples / numBins));
        float peak = 0.0f;
        for (int i = start; i < end && i < numSamples; ++i)
            peak = std::max ({ peak, std::abs (left[i]), right != nullptr ? std::abs (right[i]) : 0.0f });
        peaks[(size_t) bin] = peak;
    }
}

} // namespace oap::looper
