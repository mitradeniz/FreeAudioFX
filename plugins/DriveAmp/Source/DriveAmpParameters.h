#pragma once

#include "plugin/AmpProcessorBase.h"

/** Parameter IDs are part of saved sessions and automation - never rename them. */
namespace DriveAmpParameters
{
    inline constexpr const char* gate      = "gate";
    inline constexpr const char* tight     = "tight";
    inline constexpr const char* boost     = "boost";
    inline constexpr const char* gain      = "gain";
    inline constexpr const char* bass      = "bass";
    inline constexpr const char* middle    = "middle";
    inline constexpr const char* treble    = "treble";
    inline constexpr const char* presence  = "presence";
    inline constexpr const char* resonance = "resonance";
    inline constexpr const char* master    = "master";
    inline constexpr const char* output    = "output";
    inline constexpr const char* cabinet   = "cabinet";

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace oap::params;
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add (gateThreshold (gate, -75.0f),
                    knob (tight, "Tight", 5.0f),
                    toggle (boost, "Boost", false),
                    knob (gain, "Gain", 6.5f),
                    knob (bass, "Bass", 5.0f),
                    knob (middle, "Middle", 6.0f),
                    knob (treble, "Treble", 6.0f),
                    knob (presence, "Presence", 5.0f),
                    knob (resonance, "Resonance", 4.0f),
                    knob (master, "Master", 5.0f),
                    outputLevel (output, -24.0f, 12.0f),
                    cabinetChoice (cabinet, oap::dsp::CabinetSection::Closed4x12));
        return layout;
    }
}
