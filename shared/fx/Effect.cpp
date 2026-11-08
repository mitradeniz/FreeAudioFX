#include "Effect.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace oap::fx
{

namespace
{
    std::string fixed (float v, int decimals)
    {
        char buf[32];
        std::snprintf (buf, sizeof (buf), "%.*f", std::max (0, decimals), (double) v);
        std::string s (buf);
        if (s == "-0" || s == "-0.0" || s == "-0.00")
            s.erase (0, 1);
        return s;
    }

    // Same mapping as juce::NormalisableRange::setSkewForCentre().
    float skewFor (const ParamSpec& p)
    {
        if (p.skewCentre <= p.min || p.skewCentre >= p.max)
            return 1.0f;
        return std::log (0.5f) / std::log ((p.skewCentre - p.min) / (p.max - p.min));
    }
}

std::string ParamSpec::format (float v) const
{
    switch (type)
    {
        case ParamType::Toggle:
            return v > 0.5f ? "On" : "Off";

        case ParamType::Choice:
        {
            const auto i = (std::size_t) std::clamp ((int) std::lround (v), 0, (int) choices.size() - 1);
            return choices.empty() ? std::string() : choices[i];
        }

        case ParamType::Continuous:
        default:
            break;
    }

    if (unit == "ms" && v >= 1000.0f)
        return fixed (v / 1000.0f, 2) + " s";
    if (unit == "Hz" && v >= 1000.0f)
        return fixed (v / 1000.0f, 1) + " kHz";
    if (unit == "st" || unit == "ct" || unit == "dB")
        return (v > 0.0f && decimals == 0 ? "+" : (v > 0.04f ? "+" : "")) + fixed (v, decimals) + " " + unit;
    if (unit.empty())
        return fixed (v, decimals);
    if (unit == "%")
        return fixed (v, decimals) + "%";
    return fixed (v, decimals) + " " + unit;
}

float ParamSpec::fromNormalised (float t) const
{
    t = std::clamp (t, 0.0f, 1.0f);
    if (type == ParamType::Toggle)
        return t >= 0.5f ? 1.0f : 0.0f;
    if (type == ParamType::Choice)
        return std::round (min + t * (max - min));

    const float skew = skewFor (*this);
    const float proportion = std::abs (skew - 1.0f) < 1.0e-6f ? t : std::exp (std::log (std::max (t, 1.0e-9f)) / skew);
    return min + proportion * (max - min);
}

float ParamSpec::toNormalised (float value) const
{
    if (max <= min)
        return 0.0f;
    const float proportion = std::clamp ((value - min) / (max - min), 0.0f, 1.0f);
    if (type != ParamType::Continuous)
        return proportion;
    const float skew = skewFor (*this);
    return std::abs (skew - 1.0f) < 1.0e-6f ? proportion : std::pow (proportion, skew);
}

//==============================================================================
Effect::Effect (const EffectInfo& infoIn) : effectInfo (infoIn)
{
    for (const auto& spec : effectInfo.params)
        values.push_back (spec.def);
}

void Effect::setParameter (std::size_t index, float value) noexcept
{
    if (index < values.size())
        values[index] = value;
}

float Effect::getParameter (std::size_t index) const noexcept
{
    return index < values.size() ? values[index] : 0.0f;
}

} // namespace oap::fx
