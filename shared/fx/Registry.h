#pragma once

#include "Effect.h"

#include <functional>
#include <vector>

namespace oap::fx
{

struct EffectEntry
{
    const EffectInfo* info;
    std::unique_ptr<Effect> (*create)();
};

// Each effects source file registers its effects through one of these.
void addDynamicsAndDriveEffects (std::vector<EffectEntry>&);
void addModulationEffects (std::vector<EffectEntry>&);
void addTimeEffects (std::vector<EffectEntry>&);
void addFilterAndPitchEffects (std::vector<EffectEntry>&);

template <typename EffectType>
std::unique_ptr<Effect> makeEffect() { return std::make_unique<EffectType>(); }

} // namespace oap::fx
