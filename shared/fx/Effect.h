#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace oap::fx
{

//==============================================================================
enum class ParamType { Continuous, Choice, Toggle };

/** Describes one effect parameter. The id is part of saved sessions and
    automation, so it must never change once released. */
struct ParamSpec
{
    std::string id;
    std::string name;
    ParamType type = ParamType::Continuous;
    float min = 0.0f, max = 10.0f, def = 5.0f;
    std::string unit;                  // "", "dB", "ms", "Hz", "%", "st", "ct", "s"
    int decimals = 1;
    float skewCentre = 0.0f;           // > 0: value at the knob's centre position (log-like feel)
    std::vector<std::string> choices;  // for Choice

    static ParamSpec knob (std::string id, std::string name, float def)
    {
        ParamSpec p; p.id = std::move (id); p.name = std::move (name); p.def = def; return p;
    }

    static ParamSpec range (std::string id, std::string name, float min, float max, float def,
                            std::string unit, int decimals = 1, float skewCentre = 0.0f)
    {
        ParamSpec p; p.id = std::move (id); p.name = std::move (name);
        p.min = min; p.max = max; p.def = def; p.unit = std::move (unit);
        p.decimals = decimals; p.skewCentre = skewCentre;
        return p;
    }

    static ParamSpec choice (std::string id, std::string name, std::vector<std::string> options, int def)
    {
        ParamSpec p; p.id = std::move (id); p.name = std::move (name); p.type = ParamType::Choice;
        p.choices = std::move (options); p.min = 0.0f; p.max = (float) (p.choices.size() - 1);
        p.def = (float) def; p.decimals = 0;
        return p;
    }

    static ParamSpec toggle (std::string id, std::string name, bool def)
    {
        ParamSpec p; p.id = std::move (id); p.name = std::move (name); p.type = ParamType::Toggle;
        p.min = 0.0f; p.max = 1.0f; p.def = def ? 1.0f : 0.0f; p.decimals = 0;
        return p;
    }

    /** Human-readable value, e.g. "350 ms", "Hall", "On". */
    std::string format (float value) const;

    /** Maps a 0..1 knob position to a value (honours skew, choices and toggles). */
    float fromNormalised (float t) const;
    float toNormalised (float value) const;
};

//==============================================================================
/** Static description of an effect: what the UI, the plugin wrappers and the
    pedalboard need to know about it. */
struct EffectInfo
{
    std::string id;          // stable, e.g. "overdrive"
    std::string name;        // "Overdrive"
    std::string category;    // "Drive", "Modulation", "Delay & Reverb", ...
    std::string description; // one line for the UI and docs
    std::uint32_t colour = 0xff808080; // pedal enclosure colour (ARGB)
    bool advanced = false;
    std::vector<ParamSpec> params; // at most maxParams

    static constexpr std::size_t maxParams = 6;
};

//==============================================================================
/** Base class for every effect. Parameters are set from the audio thread
    before process(); effects smooth them internally. */
class Effect
{
public:
    explicit Effect (const EffectInfo& infoIn);
    virtual ~Effect() = default;

    const EffectInfo& info() const noexcept { return effectInfo; }

    void setParameter (std::size_t index, float value) noexcept;
    float getParameter (std::size_t index) const noexcept;
    void setTempo (double bpm) noexcept { tempo = bpm > 20.0 ? bpm : 120.0; }

    virtual void prepare (double sampleRate, int maxBlockSize) = 0;
    virtual void reset() = 0;

    /** In-place processing of 1 (mono) or 2 (stereo) channels. */
    virtual void process (float* const* channels, int numChannels, int numSamples) noexcept = 0;

    virtual int getLatencySamples() const noexcept { return 0; }
    virtual double getTailSeconds() const noexcept { return 0.0; }

protected:
    float p (std::size_t index) const noexcept { return values[index]; }
    int choiceIndex (std::size_t index) const noexcept { return (int) (values[index] + 0.5f); }
    bool toggled (std::size_t index) const noexcept { return values[index] > 0.5f; }

    double tempo = 120.0;

private:
    const EffectInfo& effectInfo;
    std::vector<float> values;
};

//==============================================================================
/** Every effect in the collection, in display order. */
const std::vector<const EffectInfo*>& allEffects();
const EffectInfo* findEffect (const std::string& id);
std::unique_ptr<Effect> createEffect (const std::string& id);

} // namespace oap::fx
