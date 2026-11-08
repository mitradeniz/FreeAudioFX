#pragma once

#include "amps/EffectAmpEngine.h"
#include "plugin/AmpProcessorBase.h"

namespace oap::amps
{

//==============================================================================
/** Plugin wrapper shared by all effect amps. Each effect amp plugin is this
    class with a different model id; parameters, editor and state handling
    come from the model description.

    Parameter IDs are part of saved sessions and automation - never rename them. */
class EffectAmpProcessor final : public AmpProcessorBase
{
public:
    static constexpr const char* gateId    = "gate";
    static constexpr const char* gainId    = "gain";
    static constexpr const char* bassId    = "bass";
    static constexpr const char* middleId  = "middle";
    static constexpr const char* trebleId  = "treble";
    static constexpr const char* masterId  = "master";
    static constexpr const char* outputId  = "output";
    static constexpr const char* cabinetId = "cabinet";

    /** "fx_" + the FX control's id, e.g. "fx_echo". */
    static juce::String fxParameterId (const fx::ParamSpec&);

    explicit EffectAmpProcessor (const std::string& modelId);
    ~EffectAmpProcessor() override;

    const AmpModel& getModel() const noexcept { return model; }

    //==============================================================================
    const juce::String getName() const override { return "OAP " + juce::String (model.name); }
    double getTailLengthSeconds() const override { return engine.getTailSeconds(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;

protected:
    dsp::CabinetSection& getCabinet() noexcept override { return engine.getCabinet(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const AmpModel&);

    const AmpModel& model;
    EffectAmpEngine engine;
    bool prepared = false;

    std::atomic<float>* gate = nullptr; // only when the model has a gate
    std::atomic<float>* gain;
    std::atomic<float>* bass;
    std::atomic<float>* middle;
    std::atomic<float>* treble;
    std::atomic<float>* master;
    std::atomic<float>* output;
    std::atomic<float>* cabinet;
    std::vector<std::atomic<float>*> fxValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectAmpProcessor)
};

} // namespace oap::amps
