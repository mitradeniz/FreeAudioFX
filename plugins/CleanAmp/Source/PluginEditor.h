#pragma once

#include "PluginProcessor.h"
#include "ui/AmpControls.h"

class CleanAmpEditor final : public oap::ui::AmpEditor
{
public:
    explicit CleanAmpEditor (CleanAmpProcessor&);
};
