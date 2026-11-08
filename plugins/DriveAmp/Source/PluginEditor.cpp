#include "PluginEditor.h"

#include "DriveAmpParameters.h"

namespace
{
namespace id = DriveAmpParameters;
using oap::ui::AmpKnob;
using oap::ui::AmpSwitch;

class DriveAmpPanel final : public oap::ui::AmpPanel
{
public:
    explicit DriveAmpPanel (DriveAmpProcessor& p)
        : AmpPanel (p, oap::ui::AmpTheme::drive(),
                    { "DRIVE", "AMP", "HIGH-GAIN TUBE AMPLIFIER" }, 1060, 360),
          gate      (p.getParameters(), id::gate, "Gate"),
          tight     (p.getParameters(), id::tight, "Tight"),
          boost     (p.getParameters(), id::boost, "Boost"),
          gain      (p.getParameters(), id::gain, "Gain"),
          bass      (p.getParameters(), id::bass, "Bass"),
          middle    (p.getParameters(), id::middle, "Middle"),
          treble    (p.getParameters(), id::treble, "Treble"),
          presence  (p.getParameters(), id::presence, "Presence"),
          resonance (p.getParameters(), id::resonance, "Resonance"),
          master    (p.getParameters(), id::master, "Master"),
          output    (p.getParameters(), id::output, "Output")
    {
        gate.slider.setTooltip ("Noise gate threshold. Fully left switches the gate off.");
        tight.slider.setTooltip ("Cuts low end before the distortion for a tighter, more focused attack.");
        boost.button.setTooltip ("Mid-hump overdrive boost in front of the amp.");
        resonance.slider.setTooltip ("Low-end thump of the power amp.");

        addGroup ("PREAMP", { &gate, &tight, &boost, &gain });
        addGroup ("TONE", { &bass, &middle, &treble });
        addGroup ("POWER AMP", { &presence, &resonance, &master });
        addGroup ("LEVEL", { &output });
    }

private:
    AmpKnob gate, tight;
    AmpSwitch boost;
    AmpKnob gain, bass, middle, treble, presence, resonance, master, output;
};
} // namespace

DriveAmpEditor::DriveAmpEditor (DriveAmpProcessor& p)
    : AmpEditor (p, oap::ui::AmpTheme::drive())
{
    setPanel (std::make_unique<DriveAmpPanel> (p));
}
