#pragma once

#include "Filters.h"

namespace oap::dsp
{

//==============================================================================
/** Input noise gate tuned for high-gain guitar.

    Peak detector with hysteresis and hold, so it opens instantly on a pick
    attack, does not chatter around the threshold, and fades out smoothly
    instead of chopping note tails.
*/
class NoiseGate
{
public:
    static constexpr double offThresholdDb = -90.0;

    void prepare (double sampleRate) noexcept
    {
        fs = sampleRate;
        detectorRelease = std::exp (-1.0 / (0.015 * fs));
        openCoeff  = 1.0 - std::exp (-1.0 / (0.0008 * fs));
        closeCoeff = 1.0 - std::exp (-1.0 / (0.060 * fs));
        holdSamples = (int) (0.045 * fs);
        reset();
    }

    void reset() noexcept
    {
        env = 0.0;
        gain = 1.0;
        isOpen = true;
        holdCounter = holdSamples;
    }

    /** Threshold in dBFS; anything at or below offThresholdDb disables the gate. */
    void setThresholdDb (double db) noexcept
    {
        enabled = db > offThresholdDb + 0.5;
        openThreshold  = std::pow (10.0, db / 20.0);
        closeThreshold = openThreshold * 0.5; // 6 dB hysteresis
    }

    void process (float* data, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const double x = std::abs ((double) data[i]);
            env = x > env ? x : env * detectorRelease;
            if (env < 1.0e-12) env = 0.0;

            if (! enabled)
            {
                isOpen = true;
            }
            else if (isOpen)
            {
                if (env >= closeThreshold)
                    holdCounter = holdSamples;
                else if (--holdCounter <= 0)
                    isOpen = false;
            }
            else if (env >= openThreshold)
            {
                isOpen = true;
                holdCounter = holdSamples;
            }

            const double target = isOpen ? 1.0 : 0.0;
            gain += (target - gain) * (isOpen ? openCoeff : closeCoeff);
            if (gain < 1.0e-5) gain = 0.0; // fully closed, avoids denormals
            data[i] = (float) (data[i] * gain);
        }
    }

    double getCurrentGain() const noexcept { return gain; }

private:
    double fs = 48000.0;
    double detectorRelease = 0.0, openCoeff = 1.0, closeCoeff = 1.0;
    double openThreshold = 0.0, closeThreshold = 0.0;
    double env = 0.0, gain = 1.0;
    int holdSamples = 0, holdCounter = 0;
    bool enabled = false, isOpen = true;
};

} // namespace oap::dsp
