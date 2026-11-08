#include "PluginProcessor.h"

#include "CleanAmpParameters.h"
#include "PluginEditor.h"

namespace id = CleanAmpParameters;

CleanAmpProcessor::CleanAmpProcessor()
    : AmpProcessorBase (id::createLayout(), id::cabinet),
      volume (rawParameter (id::volume)),
      bright (rawParameter (id::bright)),
      bass (rawParameter (id::bass)),
      middle (rawParameter (id::middle)),
      treble (rawParameter (id::treble)),
      reverb (rawParameter (id::reverb)),
      master (rawParameter (id::master)),
      output (rawParameter (id::output)),
      cabinet (rawParameter (id::cabinet))
{
}

void CleanAmpProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (engine.getLatencySamples());
    prepareBypass (sampleRate, samplesPerBlock, engine.getLatencySamples());
    prepared = true;
}

void CleanAmpProcessor::reset()
{
    if (prepared)
        engine.reset();
}

void CleanAmpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    CleanAmpEngine::Params p;
    p.volume   = volume->load();
    p.bright   = bright->load() > 0.5f;
    p.bass     = bass->load();
    p.middle   = middle->load();
    p.treble   = treble->load();
    p.reverb   = reverb->load();
    p.master   = master->load();
    p.outputDb = output->load();
    p.cabinet  = juce::roundToInt (cabinet->load());

    engine.process (buffer, juce::jmax (1, getTotalNumInputChannels()), p);
}

juce::AudioProcessorEditor* CleanAmpProcessor::createEditor()
{
    return new CleanAmpEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CleanAmpProcessor();
}
