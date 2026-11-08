#include "AmpProcessorBase.h"

namespace oap
{

AmpProcessorBase::AmpProcessorBase (juce::AudioProcessorValueTreeState::ParameterLayout layout, juce::String cabinetParameterId)
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", std::move (layout)),
      cabinetParamId (std::move (cabinetParameterId))
{
}

std::atomic<float>* AmpProcessorBase::rawParameter (const char* id) const
{
    auto* p = parameters.getRawParameterValue (id);
    jassert (p != nullptr); // unknown parameter id
    return p;
}

bool AmpProcessorBase::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    const auto monoOrStereo = [] (const juce::AudioChannelSet& s)
    {
        return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo();
    };

    return monoOrStereo (in) && monoOrStereo (out);
}

//==============================================================================
void AmpProcessorBase::prepareBypass (double sampleRate, int maxBlockSize, int latencyInSamples)
{
    bypassDelay.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, maxBlockSize), 2 });
    bypassDelay.setMaximumDelayInSamples (juce::jmax (1, latencyInSamples));
    bypassDelay.setDelay ((float) latencyInSamples);
    bypassDelay.reset();
    bypassPrepared = true;
}

void AmpProcessorBase::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numIn = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    if (bypassPrepared && getLatencySamples() > 0)
    {
        for (int ch = 0; ch < numIn; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                bypassDelay.pushSample (ch, d[i]);
                d[i] = bypassDelay.popSample (ch);
            }
        }
    }

    // a mono input feeding a stereo output is heard on both sides, like when active
    for (int ch = numIn; ch < buffer.getNumChannels(); ++ch)
    {
        if (numIn > 0)
            buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
        else
            buffer.clear (ch, 0, numSamples);
    }
}

//==============================================================================
juce::String AmpProcessorBase::loadImpulseResponse (const juce::File& file)
{
    const auto error = getCabinet().loadImpulseResponse (file);

    if (error.isEmpty())
    {
        parameters.state.setProperty (irFileProperty, file.getFullPathName(), nullptr);

        if (auto* cab = parameters.getParameter (cabinetParamId))
            cab->setValueNotifyingHost (cab->convertTo0to1 ((float) dsp::CabinetSection::UserIR));
    }

    return error;
}

void AmpProcessorBase::clearImpulseResponse()
{
    getCabinet().clearImpulseResponse();
    parameters.state.removeProperty (irFileProperty, nullptr);
}

juce::File AmpProcessorBase::getImpulseResponseFile() const
{
    const auto path = parameters.state.getProperty (irFileProperty).toString();
    return path.isNotEmpty() && juce::File::isAbsolutePath (path) ? juce::File (path) : juce::File();
}

//==============================================================================
void AmpProcessorBase::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AmpProcessorBase::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    parameters.replaceState (juce::ValueTree::fromXml (*xml));

    // Reload the impulse response referenced by the session, if it still exists.
    const auto irFile = getImpulseResponseFile();
    if (irFile.existsAsFile())
        getCabinet().loadImpulseResponse (irFile);
    else
        getCabinet().clearImpulseResponse();
}

//==============================================================================
namespace params
{
    std::unique_ptr<juce::AudioParameterFloat> knob (const char* id, const juce::String& name, float defaultValue)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), defaultValue,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })
                .withValueFromStringFunction ([] (const juce::String& s) { return juce::jlimit (0.0f, 10.0f, s.getFloatValue()); }));
    }

    std::unique_ptr<juce::AudioParameterFloat> outputLevel (const char* id, float minDb, float maxDb)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, "Output", juce::NormalisableRange<float> (minDb, maxDb, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes()
                .withLabel ("dB")
                .withStringFromValueFunction ([] (float v, int)
                {
                    return (v > 0.04f ? "+" : "") + juce::String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " dB";
                })
                .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); }));
    }

    std::unique_ptr<juce::AudioParameterFloat> gateThreshold (const char* id, float defaultDb)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, "Gate", juce::NormalisableRange<float> (-90.0f, -30.0f, 0.5f), defaultDb,
            juce::AudioParameterFloatAttributes()
                .withLabel ("dB")
                .withStringFromValueFunction ([] (float v, int) { return v <= -89.9f ? juce::String ("Off") : juce::String (v, 0) + " dB"; })
                .withValueFromStringFunction ([] (const juce::String& s)
                {
                    return s.trim().equalsIgnoreCase ("off") ? -90.0f : s.getFloatValue();
                }));
    }

    std::unique_ptr<juce::AudioParameterBool> toggle (const char* id, const juce::String& name, bool defaultValue)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, defaultValue);
    }

    std::unique_ptr<juce::AudioParameterChoice> cabinetChoice (const char* id, int defaultIndex)
    {
        return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, "Cabinet",
                                                              dsp::CabinetSection::getTypeNames(), defaultIndex);
    }
}

} // namespace oap
