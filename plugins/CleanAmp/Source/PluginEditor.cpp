#include "PluginEditor.h"

#include "CleanAmpParameters.h"

namespace
{
namespace id = CleanAmpParameters;
using oap::ui::AmpKnob;
using oap::ui::AmpSwitch;

class CleanAmpPanel final : public oap::ui::AmpPanel
{
public:
    explicit CleanAmpPanel (CleanAmpProcessor& p)
        : AmpPanel (p, oap::ui::AmpTheme::clean(), { "CLEAN", "AMP", "VINTAGE CLEAN AMPLIFIER WITH REVERB" }, 900, 360),
          volume (p.getParameters(), id::volume, "Volume"),
          bright (p.getParameters(), id::bright, "Bright"),
          bass   (p.getParameters(), id::bass, "Bass"),
          middle (p.getParameters(), id::middle, "Middle"),
          treble (p.getParameters(), id::treble, "Treble"),
          reverb (p.getParameters(), id::reverb, "Reverb"),
          master (p.getParameters(), id::master, "Master"),
          output (p.getParameters(), id::output, "Output")
    {
        volume.slider.setTooltip ("Preamp volume. Past 6 the amp starts to break up.");
        bright.button.setTooltip ("Bright cap: adds sparkle at low volume settings, fades out as Volume goes up.");
        master.slider.setTooltip ("Power amp drive. Higher settings add sag and compression.");

        addGroup ("INPUT", { &volume, &bright });
        addGroup ("TONE", { &bass, &middle, &treble });
        addGroup ("AMP", { &reverb, &master });
        addGroup ("LEVEL", { &output });
    }

private:
    AmpKnob volume;
    AmpSwitch bright;
    AmpKnob bass, middle, treble, reverb, master, output;
};
} // namespace

CleanAmpEditor::CleanAmpEditor (CleanAmpProcessor& p)
    : AmpEditor (p, oap::ui::AmpTheme::clean())
{
    setPanel (std::make_unique<CleanAmpPanel> (p));
}
