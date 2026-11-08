#include "PluginProcessor.h"

#include "DriveAmpParameters.h"
#include "PluginEditor.h"

namespace id = DriveAmpParameters;

DriveAmpProcessor::DriveAmpProcessor()
    : AmpProcessorBase (id::createLayout(), id::cabinet),
      gate (rawParameter (id::gate)),
      tight (rawParameter (id::tight)),
      boost (rawParameter (id::boost)),
      gain (rawParameter (id::gain)),
      bass (rawParameter (id::bass)),
      middle (rawParameter (id::middle)),
      treble (rawParameter (id::treble)),
      presence (rawParameter (id::presence)),
      resonance (rawParameter (id::resonance)),
      master (rawParameter (id::master)),
      output (rawParameter (id::output)),
      cabinet (rawParameter (id::cabinet))
{
}

void DriveAmpProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (engine.getLatencySamples());
    prepareBypass (sampleRate, samplesPerBlock, engine.getLatencySamples());
    prepared = true;
}

void DriveAmpProcessor::reset()
{
    if (prepared)
        engine.reset();
}

void DriveAmpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    DriveAmpEngine::Params p;
    p.gateDb    = gate->load();
    p.tight     = tight->load();
    p.boost     = boost->load() > 0.5f;
    p.gain      = gain->load();
    p.bass      = bass->load();
    p.middle    = middle->load();
    p.treble    = treble->load();
    p.presence  = presence->load();
    p.resonance = resonance->load();
    p.master    = master->load();
    p.outputDb  = output->load();
    p.cabinet   = juce::roundToInt (cabinet->load());

    engine.process (buffer, juce::jmax (1, getTotalNumInputChannels()), p);
}

juce::AudioProcessorEditor* DriveAmpProcessor::createEditor()
{
    return new DriveAmpEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DriveAmpProcessor();
}
