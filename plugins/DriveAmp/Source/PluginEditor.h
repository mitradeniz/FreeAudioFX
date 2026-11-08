#pragma once

#include "PluginProcessor.h"
#include "ui/AmpControls.h"

class DriveAmpEditor final : public oap::ui::AmpEditor
{
public:
    explicit DriveAmpEditor (DriveAmpProcessor&);
};
