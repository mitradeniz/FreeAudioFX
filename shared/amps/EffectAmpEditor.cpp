#include "EffectAmpEditor.h"

namespace oap::amps
{

ui::AmpTheme themeFor (const AmpModel& m)
{
    return ui::AmpTheme::custom (juce::Colour (m.look.background), juce::Colour (m.look.plateTop),
                                 juce::Colour (m.look.plateBottom), juce::Colour (m.look.accent),
                                 m.look.darkText);
}

namespace
{
    class EffectAmpPanel final : public ui::AmpPanel
    {
    public:
        explicit EffectAmpPanel (EffectAmpProcessor& p)
            : AmpPanel (p, themeFor (p.getModel()),
                        { p.getModel().titleAccent, p.getModel().titleRest, p.getModel().tagline },
                        widthFor (p.getModel()), 360)
        {
            auto& state = p.getParameters();
            const auto& model = p.getModel();

            std::vector<juce::Component*> amp, effects;

            if (model.voicing.hasGate)
            {
                auto* gate = add (std::make_unique<ui::AmpKnob> (state, EffectAmpProcessor::gateId, "Gate"));
                gate->slider.setTooltip ("Noise gate threshold. Fully left switches the gate off.");
                amp.push_back (gate);
            }

            for (const auto& [id, caption] : { std::pair { EffectAmpProcessor::gainId, "Gain" },
                                               std::pair { EffectAmpProcessor::bassId, "Bass" },
                                               std::pair { EffectAmpProcessor::middleId, "Middle" },
                                               std::pair { EffectAmpProcessor::trebleId, "Treble" },
                                               std::pair { EffectAmpProcessor::masterId, "Master" } })
                amp.push_back (add (std::make_unique<ui::AmpKnob> (state, id, caption)));

            for (const auto& spec : model.fxControls)
            {
                const auto id = EffectAmpProcessor::fxParameterId (spec);
                const juce::String caption (spec.name);

                switch (spec.type)
                {
                    case fx::ParamType::Toggle:
                        effects.push_back (add (std::make_unique<ui::AmpSwitch> (state, id, caption)));
                        break;
                    case fx::ParamType::Choice:
                        effects.push_back (add (std::make_unique<ui::AmpChoice> (state, id, caption)));
                        break;
                    case fx::ParamType::Continuous:
                    default:
                        effects.push_back (add (std::make_unique<ui::AmpKnob> (state, id, caption)));
                        break;
                }
            }

            auto* output = add (std::make_unique<ui::AmpKnob> (state, EffectAmpProcessor::outputId, "Output"));

            addGroup ("AMP", amp);
            addGroup ("EFFECTS", effects);
            addGroup ("LEVEL", { output });
        }

    private:
        static int widthFor (const AmpModel& m)
        {
            const int controls = (m.voicing.hasGate ? 1 : 0) + 5 + (int) m.fxControls.size() + 1;
            return juce::jmax (900, 140 + 86 * controls);
        }

        template <typename ComponentType>
        ComponentType* add (std::unique_ptr<ComponentType> c)
        {
            auto* raw = c.get();
            owned.emplace_back (std::move (c));
            return raw;
        }

        std::vector<std::unique_ptr<juce::Component>> owned;
    };
} // namespace

EffectAmpEditor::EffectAmpEditor (EffectAmpProcessor& p)
    : AmpEditor (p, themeFor (p.getModel()))
{
    setPanel (std::make_unique<EffectAmpPanel> (p));
}

} // namespace oap::amps
