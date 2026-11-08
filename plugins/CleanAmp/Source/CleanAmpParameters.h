#pragma once

#include "plugin/AmpProcessorBase.h"

/** Parameter IDs are part of saved sessions and automation - never rename them. */
namespace CleanAmpParameters
{
    inline constexpr const char* volume  = "volume";
    inline constexpr const char* bright  = "bright";
    inline constexpr const char* bass    = "bass";
    inline constexpr const char* middle  = "middle";
    inline constexpr const char* treble  = "treble";
    inline constexpr const char* reverb  = "reverb";
    inline constexpr const char* master  = "master";
    inline constexpr const char* output  = "output";
    inline constexpr const char* cabinet = "cabinet";

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace oap::params;
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add (knob (volume, "Volume", 4.5f),
                    toggle (bright, "Bright", false),
                    knob (bass, "Bass", 5.0f),
                    knob (middle, "Middle", 5.0f),
                    knob (treble, "Treble", 6.0f),
                    knob (reverb, "Reverb", 2.5f),
                    knob (master, "Master", 5.0f),
                    outputLevel (output, -24.0f, 12.0f),
                    cabinetChoice (cabinet, oap::dsp::CabinetSection::Open2x12));
        return layout;
    }
}
