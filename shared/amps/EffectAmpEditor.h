#pragma once

#include "amps/EffectAmpProcessor.h"
#include "ui/AmpControls.h"

namespace oap::amps
{

/** Theme for an effect amp, built from the model's colours. */
ui::AmpTheme themeFor (const AmpModel&);

/** Editor for every effect amp: an amp head whose faceplate is generated from
    the model (amp controls, the model's FX controls, output level). */
class EffectAmpEditor final : public ui::AmpEditor
{
public:
    explicit EffectAmpEditor (EffectAmpProcessor&);
};

} // namespace oap::amps
