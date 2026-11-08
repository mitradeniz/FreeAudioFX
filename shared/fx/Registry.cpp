#include "Registry.h"

#include <algorithm>

namespace oap::fx
{

namespace
{
    // Display order: the 12 everyday pedals first, then the 8 advanced ones.
    constexpr const char* displayOrder[] = {
        "compressor", "boost", "overdrive", "distortion", "fuzz",
        "chorus", "flanger", "phaser", "tremolo",
        "delay", "reverb", "wah",
        "shimmer", "tapeecho", "pitch", "envfilter", "rotary", "freeze", "synth", "lofi",
    };

    const std::vector<EffectEntry>& entries()
    {
        static const std::vector<EffectEntry> sorted = []
        {
            std::vector<EffectEntry> all;
            addDynamicsAndDriveEffects (all);
            addModulationEffects (all);
            addTimeEffects (all);
            addFilterAndPitchEffects (all);

            std::vector<EffectEntry> ordered;
            for (const char* id : displayOrder)
                for (const auto& e : all)
                    if (e.info->id == id)
                        ordered.push_back (e);
            return ordered;
        }();
        return sorted;
    }
}

const std::vector<const EffectInfo*>& allEffects()
{
    static const std::vector<const EffectInfo*> infos = []
    {
        std::vector<const EffectInfo*> v;
        for (const auto& e : entries())
            v.push_back (e.info);
        return v;
    }();
    return infos;
}

const EffectInfo* findEffect (const std::string& id)
{
    for (const auto& e : entries())
        if (e.info->id == id)
            return e.info;
    return nullptr;
}

std::unique_ptr<Effect> createEffect (const std::string& id)
{
    for (const auto& e : entries())
        if (e.info->id == id)
            return e.create();
    return nullptr;
}

} // namespace oap::fx
