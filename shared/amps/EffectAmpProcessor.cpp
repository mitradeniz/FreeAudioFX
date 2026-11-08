#include "EffectAmpProcessor.h"

#include "amps/EffectAmpEditor.h"
#include "plugin/FxParameters.h"

namespace oap::amps
{

namespace
{
    const AmpModel& lookup (const std::string& id)
    {
        const auto* found = findAmpModel (id);
        jassert (found != nullptr); // unknown amp model id
        return found != nullptr ? *found : allAmpModels().front();
    }
}

juce::String EffectAmpProcessor::fxParameterId (const fx::ParamSpec& spec)
{
    return "fx_" + juce::String (spec.id);
}

juce::AudioProcessorValueTreeState::ParameterLayout EffectAmpProcessor::createLayout (const AmpModel& m)
{
    using namespace oap::params;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    if (m.voicing.hasGate)
        layout.add (gateThreshold (gateId, m.voicing.gateDefaultDb));

    layout.add (knob (gainId, "Gain", m.defaultGain),
                knob (bassId, "Bass", m.defaultBass),
                knob (middleId, "Middle", m.defaultMiddle),
                knob (trebleId, "Treble", m.defaultTreble),
                knob (masterId, "Master", m.defaultMaster));

    for (const auto& spec : m.fxControls)
        layout.add (makeParameter (spec, "fx_"));

    layout.add (outputLevel (outputId, -24.0f, 12.0f),
                cabinetChoice (cabinetId, m.voicing.defaultCabinet));
    return layout;
}

EffectAmpProcessor::EffectAmpProcessor (const std::string& modelId)
    : AmpProcessorBase (createLayout (lookup (modelId)), cabinetId),
      model (lookup (modelId)),
      engine (model),
      gain (rawParameter (gainId)),
      bass (rawParameter (bassId)),
      middle (rawParameter (middleId)),
      treble (rawParameter (trebleId)),
      master (rawParameter (masterId)),
      output (rawParameter (outputId)),
      cabinet (rawParameter (cabinetId))
{
    if (model.voicing.hasGate)
        gate = rawParameter (gateId);

    for (const auto& spec : model.fxControls)
        fxValues.push_back (parameters.getRawParameterValue (fxParameterId (spec)));
}

EffectAmpProcessor::~EffectAmpProcessor() = default;

void EffectAmpProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (engine.getLatencySamples());
    prepareBypass (sampleRate, samplesPerBlock, engine.getLatencySamples());
    prepared = true;
}

void EffectAmpProcessor::reset()
{
    if (prepared)
        engine.reset();
}

void EffectAmpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    EffectAmpEngine::Params p;
    if (gate != nullptr)
        p.gateDb = gate->load();
    p.gain     = gain->load();
    p.bass     = bass->load();
    p.middle   = middle->load();
    p.treble   = treble->load();
    p.master   = master->load();
    p.outputDb = output->load();
    p.cabinet  = juce::roundToInt (cabinet->load());

    for (size_t i = 0; i < fxValues.size() && i < p.fx.size(); ++i)
        p.fx[i] = fxValues[i] != nullptr ? fxValues[i]->load() : model.fxControls[i].def;

    if (auto* head = getPlayHead())
        if (auto position = head->getPosition())
            if (auto bpm = position->getBpm())
                engine.setTempo (*bpm);

    engine.process (buffer, juce::jmax (1, getTotalNumInputChannels()), p);
}

juce::AudioProcessorEditor* EffectAmpProcessor::createEditor()
{
    return new EffectAmpEditor (*this);
}

} // namespace oap::amps
