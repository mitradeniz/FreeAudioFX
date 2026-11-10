#include "EffectAmpEditor.h"

namespace oap::amps
{

ui::AmpTheme themeFor (const AmpModel& m)
{
    auto t = ui::AmpTheme::custom (juce::Colour (m.look.background), juce::Colour (m.look.plateTop),
                                   juce::Colour (m.look.plateBottom), juce::Colour (m.look.accent),
                                   m.look.darkText);

    // Artwork: shared/art/<id>.svg, plus the ornament drawn around the controls.
    struct Style { const char* id; ui::Ornament ornament; float plateOpacity; };
    using O = ui::Ornament;
    static constexpr Style styles[] = {
        { "dream",      O::halo,      0.68f }, { "dub",       O::rays,      0.74f },
        { "spacedelay", O::orbit,     0.78f }, { "blackmetal", O::pentagram, 0.72f },
        { "warmrock",   O::ornate,    0.78f }, { "surf",      O::stitch,    0.76f },
        { "shoegaze",   O::halo,      0.70f }, { "arena",     O::neon,      0.70f },
        { "stereojazz", O::ornate,    0.74f }, { "doom",      O::thorns,    0.74f },
        { "crunch",     O::ornate,    0.74f }, { "eightbit",  O::pixel,     0.76f },
        { "knight",     O::ornate,    0.74f }, { "heavymetal", O::bolts,    0.72f },
        { "synthwave",  O::neon,      0.66f }, { "western",   O::stitch,    0.80f },
        { "funk",       O::sparkles,  0.70f }, { "pharaoh",   O::rays,      0.78f },
        { "sakura",     O::petals,    0.78f }, { "persian",   O::star,      0.74f },
        { "choir",      O::rose,      0.78f },
    };

    for (const auto& s : styles)
        if (m.id == s.id)
        {
            t.art = m.id;
            t.ornament = s.ornament;
            t.plateOpacity = s.plateOpacity;
        }

    return t;
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
            return juce::jmax (1000, 140 + 86 * controls); // wide enough for the logo, artwork and brand
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
