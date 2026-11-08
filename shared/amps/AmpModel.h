#pragma once

// Data that defines one "effect amp": the amp voicing, the built-in effects,
// the extra controls on its faceplate, and its colours. Adding a new amp to
// the collection means adding one entry to AmpModels.cpp.

#include "dsp/CabinetSection.h"
#include "dsp/ToneStack.h"
#include "dsp/TubeStage.h"
#include "fx/Effect.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace oap::amps
{

//==============================================================================
struct AmpVoicing
{
    // Linear shaping before the preamp (base rate)
    double inputHighpassHz = 30.0;
    double brightHz = 1800.0, brightDb = 0.0;     // pre-emphasis shelf
    double preMidHz = 800.0, preMidQ = 0.7, preMidDb = 0.0;

    // Preamp: up to four triode stages; the Gain knob sits after the first one.
    std::vector<dsp::TubeStage::Settings> stages;
    float gainMinDb = -30.0f, gainMaxDb = 6.0f;
    double gainCompensation = 0.0; // 0..1: how much the output level is compensated for the Gain knob
                                   // (clean amps barely compress, so without it Gain is mostly a volume control)

    // Tone stack and the EQ after it
    dsp::ToneStack::Components toneStack = dsp::ToneStack::bassman();
    double toneMakeup = 4.0;
    double postMidHz = 600.0, postMidQ = 0.8, postMidDb = 0.0;
    double lowShelfHz = 90.0, lowShelfDb = 0.0;
    double presenceDb = 0.0;
    double powerBandwidthHz = 11000.0;

    // Power amp
    float masterMinDb = -12.0f, masterMaxDb = 12.0f;
    double masterCompensation = 0.12; // like gainCompensation, for the Master knob
    double sag = 0.4;
    double outputTrim = 0.5;

    int oversamplingOrder = 2; // 2 = 4x, 3 = 8x
    bool hasGate = false;
    float gateDefaultDb = -70.0f;
    int defaultCabinet = dsp::CabinetSection::Closed4x12;
};

//==============================================================================
/** A built-in effect: which effect from the library, where it sits, and the
    parameter values that stay fixed (the rest are driven by the FX knobs). */
struct BuiltInEffect
{
    std::string effectId;
    bool beforeAmp = false;
    std::vector<std::pair<std::string, float>> fixed;
};

/** Gives the FX-knob mapping access to the amp's effects by position. */
class EffectSetter
{
public:
    /** `unknownCounter` (optional, for tests) counts calls naming an effect or parameter that doesn't exist. */
    explicit EffectSetter (std::vector<fx::Effect*>& effectsIn, int* unknownCounter = nullptr)
        : effects (effectsIn), unknown (unknownCounter) {}

    /** Sets parameter `paramId` of the effect at `index` in the model's list. */
    void set (std::size_t index, const char* paramId, float value) const noexcept
    {
        if (index < effects.size() && effects[index] != nullptr)
        {
            auto& e = *effects[index];
            const auto& specs = e.info().params;
            for (std::size_t i = 0; i < specs.size(); ++i)
                if (specs[i].id == paramId)
                {
                    e.setParameter (i, std::clamp (value, specs[i].min, specs[i].max));
                    return;
                }
        }

        if (unknown != nullptr)
            ++*unknown;
    }

private:
    std::vector<fx::Effect*>& effects;
    int* unknown = nullptr;
};

//==============================================================================
struct AmpLook
{
    std::uint32_t background, plateTop, plateBottom, accent;
    bool darkText = false;
};

struct AmpModel
{
    std::string id;           // stable, e.g. "dream"
    std::string titleAccent;  // logo, first word (accent colour)
    std::string titleRest;    // logo, rest
    std::string name;         // "Dream Amp"
    std::string tagline;      // shown under the logo
    std::string description;  // one paragraph for the docs
    AmpLook look;

    AmpVoicing voicing;
    float defaultGain = 5.0f, defaultBass = 5.0f, defaultMiddle = 5.0f, defaultTreble = 6.0f, defaultMaster = 5.0f;

    std::vector<BuiltInEffect> effects;
    std::vector<fx::ParamSpec> fxControls; // 2-5 extra knobs, ids are prefixed with "fx_"

    /** Called once per block with the current FX-knob values (same order as fxControls). */
    std::function<void (const float* fxValues, const EffectSetter&)> applyControls;
};

const std::vector<AmpModel>& allAmpModels();
const AmpModel* findAmpModel (const std::string& id);

} // namespace oap::amps
