#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "fx/Effect.h"

namespace oap
{

/** Turns an effect parameter description into a JUCE parameter. `idPrefix`
    and `namePrefix` let the effect amps and the Pedalboard namespace them. */
inline std::unique_ptr<juce::RangedAudioParameter> makeParameter (const fx::ParamSpec& spec,
                                                                   const juce::String& idPrefix = {},
                                                                   const juce::String& namePrefix = {})
{
    const juce::ParameterID id { idPrefix + juce::String (spec.id), 1 };
    const juce::String name = namePrefix + juce::String (spec.name);

    switch (spec.type)
    {
        case fx::ParamType::Toggle:
            return std::make_unique<juce::AudioParameterBool> (id, name, spec.def > 0.5f);

        case fx::ParamType::Choice:
        {
            juce::StringArray choices;
            for (const auto& c : spec.choices)
                choices.add (c);
            return std::make_unique<juce::AudioParameterChoice> (id, name, choices, (int) spec.def);
        }

        case fx::ParamType::Continuous:
        default:
            break;
    }

    // Whole-number parameters (semitones, bits, milliseconds, percent) snap to integers.
    juce::NormalisableRange<float> range (spec.min, spec.max, spec.decimals <= 0 ? 1.0f : 0.0f);
    if (spec.skewCentre > spec.min && spec.skewCentre < spec.max)
        range.setSkewForCentre (spec.skewCentre);

    auto attributes = juce::AudioParameterFloatAttributes()
                          .withLabel (spec.unit)
                          .withStringFromValueFunction ([spec] (float v, int) { return juce::String (spec.format (v)); })
                          .withValueFromStringFunction ([] (const juce::String& s) { return s.retainCharacters ("-0123456789.").getFloatValue(); });

    return std::make_unique<juce::AudioParameterFloat> (id, name, range, spec.def, attributes);
}

} // namespace oap
