#include "PedalboardProcessor.h"

#include "ui/PedalboardEditor.h"

#include <map>

namespace oap
{

namespace
{
    // The board you get when you first load the plugin.
    constexpr const char* defaultBoard[PedalboardProcessor::numSlots] = {
        "compressor", "overdrive", "chorus", "delay", "reverb", "", "", ""
    };

    int choiceIndexFor (const char* effectId)
    {
        const auto& all = fx::allEffects();
        for (size_t i = 0; i < all.size(); ++i)
            if (all[i]->id == effectId)
                return (int) i + 1;
        return 0;
    }

    // An empty slot is handed to the audio thread as this tag value, so that
    // "nothing pending" (nullptr) and "switch to empty" can be told apart.
    fx::Effect* const emptyTag = reinterpret_cast<fx::Effect*> (std::uintptr_t (1));

    void deleteEffect (fx::Effect* e)
    {
        if (e != nullptr && e != emptyTag)
            delete e;
    }
}

//==============================================================================
juce::String PedalboardProcessor::typeId (int slot)            { return "slot" + juce::String (slot + 1) + "_type"; }
juce::String PedalboardProcessor::onId (int slot)              { return "slot" + juce::String (slot + 1) + "_on"; }
juce::String PedalboardProcessor::knobId (int slot, int knob)  { return "slot" + juce::String (slot + 1) + "_k" + juce::String (knob + 1); }

juce::AudioProcessorValueTreeState::ParameterLayout PedalboardProcessor::createLayout (PedalboardProcessor& self)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray names { "Empty" };
    for (const auto* info : fx::allEffects())
        names.add (info->name);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "input", 1 }, "Input", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })));

    for (int s = 0; s < numSlots; ++s)
    {
        const auto prefix = "Slot " + juce::String (s + 1) + " ";
        const int defaultChoice = choiceIndexFor (defaultBoard[s]);
        const fx::EffectInfo* defaultInfo = defaultChoice > 0 ? fx::allEffects()[(size_t) defaultChoice - 1] : nullptr;

        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { typeId (s), 1 }, prefix + "Pedal", names, defaultChoice));
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { onId (s), 1 }, prefix + "On", true));

        for (int k = 0; k < knobsPerSlot; ++k)
        {
            float def = 0.5f;
            if (defaultInfo != nullptr && (size_t) k < defaultInfo->params.size())
                def = defaultInfo->params[(size_t) k].toNormalised (defaultInfo->params[(size_t) k].def);

            auto text = [&self, s, k] (float v, int) -> juce::String
            {
                if (const auto* info = self.getSlotEffect (s))
                    if ((size_t) k < info->params.size())
                    {
                        const auto& spec = info->params[(size_t) k];
                        return spec.format (spec.fromNormalised (v));
                    }
                return "-";
            };

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { knobId (s, k), 1 }, prefix + "Knob " + juce::String (k + 1),
                juce::NormalisableRange<float> (0.0f, 1.0f), def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (text)));
        }
    }
    return layout;
}

PedalboardProcessor::PedalboardProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createLayout (*this))
{
    for (int s = 0; s < numSlots; ++s)
    {
        auto& slot = slots[(size_t) s];
        slot.type = parameters.getRawParameterValue (typeId (s));
        slot.on = parameters.getRawParameterValue (onId (s));
        for (int k = 0; k < knobsPerSlot; ++k)
            slot.knobs[(size_t) k] = parameters.getRawParameterValue (knobId (s, k));
    }
    inputGain = parameters.getRawParameterValue ("input");
    outputGain = parameters.getRawParameterValue ("output");

    startTimerHz (30);
}

PedalboardProcessor::~PedalboardProcessor()
{
    stopTimer();
    for (auto& slot : slots)
    {
        deleteEffect (slot.current);
        deleteEffect (slot.incoming.exchange (nullptr));
        deleteEffect (slot.outgoing.exchange (nullptr));
    }
}

const fx::EffectInfo* PedalboardProcessor::getSlotEffect (int slot) const noexcept
{
    if (slot < 0 || slot >= numSlots || slots[(size_t) slot].type == nullptr)
        return nullptr;
    const int index = effectIndexForChoice ((int) std::lround (slots[(size_t) slot].type->load()));
    const auto& all = fx::allEffects();
    return index >= 0 && index < (int) all.size() ? all[(size_t) index] : nullptr;
}

//==============================================================================
void PedalboardProcessor::setSlotEffect (int slot, int effectIndex)
{
    auto* typeParam = parameters.getParameter (typeId (slot));
    typeParam->setValueNotifyingHost (typeParam->convertTo0to1 ((float) (effectIndex + 1)));

    const auto& all = fx::allEffects();
    if (effectIndex < 0 || effectIndex >= (int) all.size())
        return;

    const auto& specs = all[(size_t) effectIndex]->params;
    for (int k = 0; k < knobsPerSlot; ++k)
    {
        auto* knob = parameters.getParameter (knobId (slot, k));
        const float v = (size_t) k < specs.size() ? specs[(size_t) k].toNormalised (specs[(size_t) k].def) : 0.5f;
        knob->setValueNotifyingHost (v);
    }
}

void PedalboardProcessor::swapSlots (int a, int b)
{
    if (a < 0 || b < 0 || a >= numSlots || b >= numSlots || a == b)
        return;

    const auto swapParam = [this] (const juce::String& idA, const juce::String& idB)
    {
        auto* pa = parameters.getParameter (idA);
        auto* pb = parameters.getParameter (idB);
        const float va = pa->getValue(), vb = pb->getValue();
        pa->setValueNotifyingHost (vb);
        pb->setValueNotifyingHost (va);
    };

    swapParam (typeId (a), typeId (b));
    swapParam (onId (a), onId (b));
    for (int k = 0; k < knobsPerSlot; ++k)
        swapParam (knobId (a, k), knobId (b, k));
}

//==============================================================================
std::unique_ptr<fx::Effect> PedalboardProcessor::makePreparedEffect (const fx::EffectInfo& info) const
{
    auto effect = fx::createEffect (info.id);
    if (effect != nullptr)
    {
        effect->prepare (currentSampleRate.load(), currentBlockSize.load());
        effect->reset();
    }
    return effect;
}

void PedalboardProcessor::syncEffects (bool audioIsRunning)
{
    for (int s = 0; s < numSlots; ++s)
    {
        auto& slot = slots[(size_t) s];
        const auto* wanted = getSlotEffect (s);

        if (! audioIsRunning)
        {
            // prepareToPlay(): no audio callback can run, so swap directly.
            deleteEffect (slot.current);
            slot.current = wanted != nullptr ? makePreparedEffect (*wanted).release() : nullptr;
            slot.requested = wanted;
            slot.latency = slot.current != nullptr ? slot.current->getLatencySamples() : 0;
            slot.fadeIn = false;
            continue;
        }

        if (wanted == slot.requested.load())
            continue;

        // Only hand over a new effect once the audio thread has taken the previous one.
        if (slot.incoming.load() != nullptr || slot.outgoing.load() != nullptr)
            continue;

        auto fresh = wanted != nullptr ? makePreparedEffect (*wanted) : nullptr;
        slot.incoming.store (fresh != nullptr ? fresh.release() : emptyTag, std::memory_order_release);
        slot.requested = wanted;
    }
}

void PedalboardProcessor::collectGarbage()
{
    for (auto& slot : slots)
        deleteEffect (slot.outgoing.exchange (nullptr));
}

void PedalboardProcessor::updateLatency()
{
    // Only the oversampled drive pedals add latency; it doesn't depend on the
    // sample rate, so it is measured once per effect type.
    static std::map<std::string, int> latencyById;

    int total = 0;
    for (const auto& slot : slots)
        if (const auto* info = slot.requested.load())
        {
            auto it = latencyById.find (info->id);
            if (it == latencyById.end())
            {
                auto probe = fx::createEffect (info->id);
                probe->prepare (48000.0, 64);
                it = latencyById.emplace (info->id, probe->getLatencySamples()).first;
            }
            total += it->second;
        }

    if (total != getLatencySamples())
        setLatencySamples (total);
}

void PedalboardProcessor::timerCallback()
{
    const juce::ScopedLock sl (managementLock);
    collectGarbage();
    if (running.load())
    {
        syncEffects (true);
        updateLatency();
    }
}

//==============================================================================
bool PedalboardProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    const auto ok = [] (const juce::AudioChannelSet& s)
    {
        return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo();
    };
    return ok (in) && ok (out) && in.size() <= out.size();
}

void PedalboardProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::ScopedLock sl (managementLock);
    running = false;
    currentSampleRate = sampleRate;
    currentBlockSize = juce::jmax (1, samplesPerBlock);

    collectGarbage();
    for (auto& slot : slots)
    {
        deleteEffect (slot.incoming.exchange (nullptr));
        for (auto& d : slot.dryDelay)
            d.prepare (128 + 8);
        slot.wet.reset (sampleRate, 0.02);
        slot.wet.setCurrentAndTargetValue (slot.on->load() > 0.5f ? 1.0f : 0.0f);
    }

    syncEffects (false);
    updateLatency();

    dryBuffer.setSize (2, currentBlockSize.load());
    inGain.reset (sampleRate, 0.03);
    outGain.reset (sampleRate, 0.03);
    inGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (inputGain->load()));
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputGain->load()));
    running = true;
}

void PedalboardProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (n == 0 || numCh == 0)
        return;

    for (int ch = juce::jmax (1, numIn); ch < numCh; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, n);

    double bpm = 120.0;
    if (auto* head = getPlayHead())
        if (auto position = head->getPosition())
            if (auto hostBpm = position->getBpm())
                bpm = *hostBpm;

    // Pick up effects created on the message thread.
    for (auto& slot : slots)
    {
        if (slot.outgoing.load (std::memory_order_acquire) != nullptr)
            continue; // the previous effect hasn't been collected yet
        if (auto* message = slot.incoming.exchange (nullptr, std::memory_order_acq_rel))
        {
            auto* old = slot.current;
            slot.current = message == emptyTag ? nullptr : message;
            slot.outgoing.store (old != nullptr ? old : emptyTag, std::memory_order_release);
            slot.latency = slot.current != nullptr ? slot.current->getLatencySamples() : 0;
            for (auto& d : slot.dryDelay)
                d.reset();
            slot.fadeIn = true;
        }
    }

    const auto applyGain = [&buffer, numCh, n] (juce::SmoothedValue<float>& gain)
    {
        for (int i = 0; i < n; ++i)
        {
            const float g = gain.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
                buffer.getWritePointer (ch)[i] *= g;
        }
    };

    inGain.setTargetValue (juce::Decibels::decibelsToGain (inputGain->load()));
    outGain.setTargetValue (juce::Decibels::decibelsToGain (outputGain->load()));
    applyGain (inGain);

    for (auto& slot : slots)
    {
        auto* fx = slot.current;
        if (fx == nullptr)
            continue;

        const auto& specs = fx->info().params;
        for (size_t k = 0; k < specs.size(); ++k)
            fx->setParameter (k, specs[k].fromNormalised (slot.knobs[k]->load()));
        fx->setTempo (bpm);

        const float target = slot.on->load() > 0.5f ? 1.0f : 0.0f;
        if (slot.fadeIn)
        {
            slot.wet.setCurrentAndTargetValue (0.0f);
            slot.fadeIn = false;
        }
        slot.wet.setTargetValue (target);

        for (int start = 0; start < n; start += dryBuffer.getNumSamples())
        {
            const int len = juce::jmin (dryBuffer.getNumSamples(), n - start);

            for (int ch = 0; ch < numCh; ++ch)
            {
                const float* in = buffer.getReadPointer (ch, start);
                float* dry = dryBuffer.getWritePointer (ch);
                auto& line = slot.dryDelay[ch];
                for (int i = 0; i < len; ++i)
                {
                    line.push (in[i]);
                    dry[i] = slot.latency > 0 ? line.tap ((size_t) slot.latency) : in[i];
                }
            }

            if (! slot.wet.isSmoothing() && slot.wet.getTargetValue() < 0.5f)
            {
                for (int ch = 0; ch < numCh; ++ch)
                    buffer.copyFrom (ch, start, dryBuffer, ch, 0, len);
                continue;
            }

            float* chans[2] = { buffer.getWritePointer (0, start), numCh > 1 ? buffer.getWritePointer (1, start) : nullptr };
            fx->process (chans, numCh, len);

            if (slot.wet.isSmoothing())
                for (int i = 0; i < len; ++i)
                {
                    const float w = slot.wet.getNextValue();
                    for (int ch = 0; ch < numCh; ++ch)
                        chans[ch][i] = dryBuffer.getSample (ch, i) + w * (chans[ch][i] - dryBuffer.getSample (ch, i));
                }
        }
    }

    applyGain (outGain);

    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto range = juce::FloatVectorOperations::findMinAndMax (buffer.getReadPointer (ch), n);
        if (! std::isfinite (range.getStart()) || ! std::isfinite (range.getEnd()))
        {
            buffer.clear();
            for (auto& slot : slots)
                if (slot.current != nullptr)
                    slot.current->reset();
            break;
        }
    }
}

juce::AudioProcessorEditor* PedalboardProcessor::createEditor()
{
    return new ui::PedalboardEditor (*this);
}

void PedalboardProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PedalboardProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace oap
