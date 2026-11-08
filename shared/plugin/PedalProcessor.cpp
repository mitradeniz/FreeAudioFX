#include "PedalProcessor.h"

#include "plugin/FxParameters.h"
#include "ui/PedalEditor.h"

namespace oap
{

namespace
{
    const fx::EffectInfo& lookup (const std::string& id)
    {
        const auto* found = fx::findEffect (id);
        jassert (found != nullptr); // unknown effect id
        return found != nullptr ? *found : *fx::allEffects().front();
    }
}

PedalProcessor::PedalProcessor (const std::string& effectId)
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      info (lookup (effectId)),
      effect (fx::createEffect (info.id)),
      parameters (*this, nullptr, "PARAMETERS", createLayout (info))
{
    for (const auto& spec : info.params)
        values.push_back (parameters.getRawParameterValue (spec.id));
    bypass = parameters.getRawParameterValue (bypassId);
}

PedalProcessor::~PedalProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout PedalProcessor::createLayout (const fx::EffectInfo& fxInfo)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& spec : fxInfo.params)
        layout.add (makeParameter (spec));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { bypassId, 1 }, "Bypass", false));
    return layout;
}

juce::AudioProcessorParameter* PedalProcessor::getBypassParameter() const
{
    return parameters.getParameter (bypassId);
}

bool PedalProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    const auto ok = [] (const juce::AudioChannelSet& s)
    {
        return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo();
    };
    return ok (in) && ok (out) && in.size() <= out.size();
}

void PedalProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    effect->prepare (sampleRate, samplesPerBlock);
    const int latency = effect->getLatencySamples();
    setLatencySamples (latency);

    for (auto& d : dryDelay)
        d.prepare (latency + samplesPerBlock + 8);
    dryBuffer.setSize (2, samplesPerBlock);
    wetAmount.reset (sampleRate, 0.02);
    wetAmount.setCurrentAndTargetValue (bypass->load() > 0.5f ? 0.0f : 1.0f);
    prepared = true;
}

void PedalProcessor::reset()
{
    if (! prepared)
        return;
    effect->reset();
    for (auto& d : dryDelay)
        d.reset();
}

void PedalProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (n == 0 || numCh == 0)
        return;

    // A mono input on a stereo output: feed both sides so stereo effects can spread it.
    for (int ch = juce::jmax (1, numIn); ch < numCh; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, n);

    for (size_t i = 0; i < values.size(); ++i)
        effect->setParameter (i, values[i]->load());

    if (auto* head = getPlayHead())
        if (auto position = head->getPosition())
            if (auto bpm = position->getBpm())
                effect->setTempo (*bpm);

    wetAmount.setTargetValue (bypass->load() > 0.5f ? 0.0f : 1.0f);
    const int latency = getLatencySamples();

    for (int start = 0; start < n; start += dryBuffer.getNumSamples())
    {
        const int len = juce::jmin (dryBuffer.getNumSamples(), n - start);

        // Keep a latency-aligned copy of the input for bypass crossfades.
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* in = buffer.getReadPointer (ch, start);
            float* dry = dryBuffer.getWritePointer (ch);
            auto& line = dryDelay[(size_t) ch];
            for (int i = 0; i < len; ++i)
            {
                line.push (in[i]);
                dry[i] = latency > 0 ? line.tap ((size_t) latency) : in[i];
            }
        }

        const bool fullyBypassed = ! wetAmount.isSmoothing() && wetAmount.getTargetValue() < 0.5f;
        if (fullyBypassed)
        {
            for (int ch = 0; ch < numCh; ++ch)
                buffer.copyFrom (ch, start, dryBuffer, ch, 0, len);
            wasBypassed = true;
            continue;
        }

        if (wasBypassed)
        {
            // Coming back from bypass: start from a clean slate, like switching a pedal on.
            effect->reset();
            wasBypassed = false;
        }

        float* chans[2] = { buffer.getWritePointer (0, start), numCh > 1 ? buffer.getWritePointer (1, start) : nullptr };
        effect->process (chans, numCh, len);

        if (wetAmount.isSmoothing())
        {
            for (int i = 0; i < len; ++i)
            {
                const float w = wetAmount.getNextValue();
                for (int ch = 0; ch < numCh; ++ch)
                    chans[ch][i] = dryBuffer.getSample (ch, i) + w * (chans[ch][i] - dryBuffer.getSample (ch, i));
            }
        }
    }

    // Safety: never pass non-finite values to the host.
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto range = juce::FloatVectorOperations::findMinAndMax (buffer.getReadPointer (ch), n);
        if (! std::isfinite (range.getStart()) || ! std::isfinite (range.getEnd()))
        {
            buffer.clear();
            effect->reset();
            break;
        }
    }
}

juce::AudioProcessorEditor* PedalProcessor::createEditor()
{
    return new ui::PedalEditor (*this);
}

void PedalProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PedalProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace oap
