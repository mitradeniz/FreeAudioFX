// Dynamics and drive pedals: Compressor, Boost, Overdrive, Distortion, Fuzz.

#include "FxEngines.h"
#include "Registry.h"

#include <array>

namespace oap::fx
{
namespace
{

using oap::dsp::Biquad;
using oap::dsp::BiquadCoeffs;

//==============================================================================
/** Optical-style compressor with parallel blend. */
class Compressor final : public Effect
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "compressor", "Compressor", "Dynamics",
            "Evens out your picking and adds sustain, with a blend knob for natural attack.",
            0xff2d6cdf, false,
            {
                ParamSpec::knob ("sustain", "Sustain", 5.0f),
                ParamSpec::knob ("attack", "Attack", 4.0f),
                ParamSpec::knob ("tone", "Tone", 5.0f),
                ParamSpec::range ("blend", "Blend", 0.0f, 100.0f, 80.0f, "%", 0),
                ParamSpec::range ("level", "Level", -12.0f, 12.0f, 0.0f, "dB", 1),
            } };
        return info;
    }

    Compressor() : Effect (describe()) {}

    void prepare (double sampleRate, int) override
    {
        fs = sampleRate;
        releaseCoeff = (float) std::exp (-1.0 / (0.18 * fs));
        gainSmooth.prepare (fs, 2.0);
        levelSmooth.prepare (fs, 20.0);
        reset();
    }

    void reset() override
    {
        env = 0.0f;
        gainSmooth.reset (1.0f);
        levelSmooth.reset (1.0f);
        for (auto& t : tone) t.reset();
        lastTone = -1.0f;
    }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        const float sustain = p (0) / 10.0f;
        const float thresholdDb = -8.0f - 34.0f * sustain;
        const float ratio = 2.0f + 8.0f * sustain;
        const float knee = 6.0f;
        const float attackMs = 0.8f + p (1) * p (1) * 0.6f; // 0.8 .. 60 ms
        const float attackCoeff = (float) std::exp (-1.0 / (attackMs * 0.001 * fs));
        const float makeupDb = -thresholdDb * (1.0f - 1.0f / ratio) * 0.4f;
        const float blend = p (3) / 100.0f;
        const float level = dbToGain (p (4));

        if (hasChanged (p (2), lastTone))
        {
            lastTone = p (2);
            for (auto& t : tone)
                t.setCoeffs (BiquadCoeffs::highShelf (fs, 2500.0, 0.7, (p (2) - 5.0) * 1.6));
        }

        for (int i = 0; i < n; ++i)
        {
            float peak = 0.0f;
            for (int c = 0; c < numCh; ++c)
                peak = std::max (peak, std::abs (ch[c][i]));

            env = peak > env ? peak + attackCoeff * (env - peak) : peak + releaseCoeff * (env - peak);
            env = flushDenormal (env);

            const float levelDb = 20.0f * std::log10 (env + 1.0e-9f);
            const float over = levelDb - thresholdDb;
            float reductionDb = 0.0f;
            if (over > knee * 0.5f)
                reductionDb = over * (1.0f - 1.0f / ratio);
            else if (over > -knee * 0.5f)
                reductionDb = (1.0f - 1.0f / ratio) * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);

            const float g = gainSmooth.next (dbToGain (makeupDb - reductionDb));
            const float out = levelSmooth.next (level);

            for (int c = 0; c < numCh; ++c)
            {
                const float dry = ch[c][i];
                const float wet = (float) tone[(size_t) c].process (dry * g);
                ch[c][i] = (dry * (1.0f - blend) + wet * blend) * out;
            }
        }
    }

private:
    static bool hasChanged (float a, float b) noexcept { return std::abs (a - b) > 1.0e-4f; }

    double fs = 48000.0;
    float env = 0.0f, releaseCoeff = 0.0f, lastTone = -1.0f;
    Smoother gainSmooth, levelSmooth;
    std::array<Biquad, 2> tone;
};

//==============================================================================
/** Shared plumbing for the oversampled drive pedals. */
class DrivePedal : public Effect
{
public:
    using Effect::Effect;

    void prepare (double sampleRate, int maxBlockSize) override
    {
        fs = sampleRate;
        maxBlock = std::max (1, maxBlockSize);
        for (auto& c : channels)
        {
            c.os.prepare (4, chunk);
            c.dryDelay.prepare (128);
        }
        latency = channels[0].os.getLatency();
        scratch.assign ((size_t) chunk, 0.0f);
        driveSmooth.prepare (fs, 30.0);
        levelSmooth.prepare (fs, 30.0);
        prepareTone();
        reset();
    }

    void reset() override
    {
        for (auto& c : channels)
        {
            c.os.reset();
            c.dryDelay.reset();
            c.preHp.reset(); c.preLp.reset(); c.post1.reset(); c.post2.reset(); c.dc.reset();
            c.pre.reset(); c.post.reset();
        }
        driveSmooth.reset (driveTarget());
        levelSmooth.reset (levelTarget());
    }

    int getLatencySamples() const noexcept override { return latency; }

    void process (float* const* ch, int numCh, int n) noexcept override
    {
        updateFilters();

        for (int start = 0; start < n; start += chunk)
        {
            const int len = std::min (chunk, n - start);
            const float driveStart = driveSmooth.current(), levelStart = levelSmooth.current();
            float driveEnd = driveStart, levelEnd = levelStart;
            for (int i = 0; i < len; ++i)
            {
                driveEnd = driveSmooth.next (driveTarget());
                levelEnd = levelSmooth.next (levelTarget());
            }

            for (int c = 0; c < numCh; ++c)
            {
                auto& st = channels[(size_t) c];
                float* x = ch[c] + start;

                for (int i = 0; i < len; ++i)
                {
                    const float t = (float) (i + 1) / (float) len;
                    const float drive = lerp (driveStart, driveEnd, t);
                    st.dryDelay.push (x[i]);
                    scratch[(size_t) i] = preShape (st, x[i]) * drive;
                }

                float* up = st.os.up (scratch.data(), len);
                const int m = len * st.os.getFactor();
                for (int i = 0; i < m; ++i)
                    up[i] = clip (up[i]);
                st.os.down (scratch.data(), len);

                for (int i = 0; i < len; ++i)
                {
                    const float t = (float) (i + 1) / (float) len;
                    x[i] = postShape (st, scratch[(size_t) i], dryAligned (st, i, len)) * lerp (levelStart, levelEnd, t);
                }
            }
        }
    }

protected:
    struct Channel
    {
        Oversampler os;
        DelayLine dryDelay;
        OnePoleHP preHp, dc;
        OnePoleLP preLp, post1, post2;
        Biquad pre, post;
    };

    virtual void prepareTone() {}
    virtual void updateFilters() {}
    virtual float driveTarget() const noexcept = 0;
    virtual float levelTarget() const noexcept = 0;
    virtual float preShape (Channel&, float x) noexcept = 0;
    virtual float clip (float x) noexcept = 0;
    virtual float postShape (Channel&, float wet, float dry) noexcept = 0;

    /** The dry input delayed to line up with the oversampled path. */
    float dryAligned (Channel& st, int i, int len) const noexcept
    {
        return st.dryDelay.tap ((size_t) (latency + (len - 1 - i)));
    }

    static bool changed (float& last, float now) noexcept
    {
        if (std::abs (last - now) < 1.0e-4f) return false;
        last = now;
        return true;
    }

    double fs = 48000.0;
    int maxBlock = 512, latency = 0;
    static constexpr int chunk = 64;
    std::array<Channel, 2> channels;
    std::vector<float> scratch;
    Smoother driveSmooth, levelSmooth;
};

//==============================================================================
class Boost final : public DrivePedal
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "boost", "Boost", "Drive",
            "Clean boost with bass and treble shaping; turn up Push to drive your amp harder.",
            0xffb8bcc4, false,
            {
                ParamSpec::range ("gain", "Gain", 0.0f, 24.0f, 9.0f, "dB", 1),
                ParamSpec::range ("bass", "Bass", -10.0f, 10.0f, 0.0f, "dB", 1),
                ParamSpec::range ("treble", "Treble", -10.0f, 10.0f, 0.0f, "dB", 1),
                ParamSpec::knob ("push", "Push", 2.0f),
            } };
        return info;
    }

    Boost() : DrivePedal (describe()) {}

private:
    void updateFilters() override
    {
        if (changed (lastBass, p (1)))
            for (auto& c : channels) c.pre.setCoeffs (BiquadCoeffs::lowShelf (fs, 120.0, 0.7, p (1)));
        if (changed (lastTreble, p (2)))
            for (auto& c : channels) c.post.setCoeffs (BiquadCoeffs::highShelf (fs, 3000.0, 0.7, p (2)));
        headroom = dbToGain (6.0f - 1.2f * p (3));
    }

    float driveTarget() const noexcept override { return dbToGain (p (0)); }
    float levelTarget() const noexcept override { return 1.0f; }
    float preShape (Channel& c, float x) noexcept override { return (float) c.post.process (c.pre.process (x)); }
    float clip (float x) noexcept override { return headroom * softClip (x / headroom); }
    float postShape (Channel&, float wet, float) noexcept override { return wet; }

    float lastBass = -99.0f, lastTreble = -99.0f, headroom = 2.0f;
};

//==============================================================================
/** Mid-hump overdrive: the classic "screamer" topology adds clipped
    high-passed signal on top of the clean signal. */
class Overdrive final : public DrivePedal
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "overdrive", "Overdrive", "Drive",
            "Warm, mid-focused overdrive. Great on its own or to push an amp into tight saturation.",
            0xff2e9e5b, false,
            {
                ParamSpec::knob ("drive", "Drive", 5.0f),
                ParamSpec::knob ("tone", "Tone", 5.0f),
                ParamSpec::knob ("level", "Level", 5.0f),
                ParamSpec::choice ("mode", "Mode", { "Classic", "Open" }, 0),
            } };
        return info;
    }

    Overdrive() : DrivePedal (describe()) {}

private:
    void updateFilters() override
    {
        const bool open = choiceIndex (3) == 1;
        if (changed (lastMode, (float) choiceIndex (3)))
            for (auto& c : channels) c.preHp.set (open ? 220.0f : 720.0f, fs);
        if (changed (lastTone, p (1)))
            for (auto& c : channels)
            {
                c.post1.set (expMap (p (1) / 10.0f, 650.0f, 7000.0f), fs);
                c.post2.set (9500.0f, fs);
            }
        dryAmount = open ? 1.0f : 0.8f;
        clipAmount = open ? 0.38f : 0.5f;
    }

    float driveTarget() const noexcept override { return dbToGain (18.0f + 2.6f * p (0)); }
    float levelTarget() const noexcept override { return dbToGain (-29.0f + 4.2f * p (2)); }
    float preShape (Channel& c, float x) noexcept override { return c.preHp.process (x); }
    float clip (float x) noexcept override { return softClip (x); }

    float postShape (Channel& c, float wet, float dry) noexcept override
    {
        const float y = dry * dryAmount + wet * clipAmount;
        return c.post2.process (c.post1.process (y));
    }

    float lastMode = -1.0f, lastTone = -1.0f, dryAmount = 0.8f, clipAmount = 0.5f;
};

//==============================================================================
/** Op-amp hard clipper with a sweepable filter (think classic black box). */
class Distortion final : public DrivePedal
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "distortion", "Distortion", "Drive",
            "Aggressive hard-clipping distortion with a filter that tames the fizz. Choose silicon, LED or asymmetric clipping.",
            0xff26282d, false,
            {
                ParamSpec::knob ("distortion", "Distortion", 5.0f),
                ParamSpec::knob ("filter", "Filter", 4.0f),
                ParamSpec::knob ("level", "Level", 5.0f),
                ParamSpec::choice ("clip", "Clip", { "Silicon", "LED", "Asymmetric" }, 0),
            } };
        return info;
    }

    Distortion() : DrivePedal (describe()) {}

private:
    void prepareTone() override
    {
        for (auto& c : channels)
        {
            c.preHp.set (60.0f, fs);
            c.pre.setCoeffs (BiquadCoeffs::peak (fs, 1400.0, 0.7, 6.0));
            c.dc.set (15.0f, fs);
        }
    }

    void updateFilters() override
    {
        if (changed (lastDist, p (0)))
        {
            const float gain = dbToGain (10.0f + 5.5f * p (0));
            for (auto& c : channels) c.preLp.set (std::clamp (380000.0f / gain, 2500.0f, 18000.0f), fs);
        }
        if (changed (lastFilter, p (1)))
            for (auto& c : channels) c.post1.set (expMap (1.0f - p (1) / 10.0f, 550.0f, 18000.0f), fs);
        mode = choiceIndex (3);
    }

    float driveTarget() const noexcept override { return dbToGain (10.0f + 5.5f * p (0)); }
    float levelTarget() const noexcept override { return dbToGain (-34.0f + 4.4f * p (2)); }

    float preShape (Channel& c, float x) noexcept override
    {
        return c.preLp.process ((float) c.pre.process (c.preHp.process (x)));
    }

    float clip (float x) noexcept override
    {
        switch (mode)
        {
            case 1: return 0.75f * softClip (x / 1.6f);                                  // LED: more headroom, softer
            case 2: return x > 0.0f ? 0.5f * hard (x) : 0.8f * hard (x / 1.6f);           // asymmetric
            default: return 0.55f * hard (x);                                             // silicon
        }
    }

    float postShape (Channel& c, float wet, float) noexcept override { return c.post1.process (c.dc.process (wet)); }

    static float hard (float x) noexcept
    {
        const float a = std::abs (x);
        return x / std::pow (1.0f + a * a * std::sqrt (a), 0.4f); // x / (1 + |x|^2.5)^(1/2.5)
    }

    float lastDist = -1.0f, lastFilter = -1.0f;
    int mode = 0;
};

//==============================================================================
/** Two-stage transistor fuzz with a scooped tone control and a bias knob
    that goes from smooth to sputtery, gated "dying battery" fuzz. */
class Fuzz final : public DrivePedal
{
public:
    static const EffectInfo& describe()
    {
        static const EffectInfo info {
            "fuzz", "Fuzz", "Drive",
            "Thick, woolly fuzz. Bias starves the transistors for a gated, sputtery sound.",
            0xffd9472b, false,
            {
                ParamSpec::knob ("fuzz", "Fuzz", 6.0f),
                ParamSpec::knob ("tone", "Tone", 5.0f),
                ParamSpec::knob ("volume", "Volume", 5.0f),
                ParamSpec::knob ("bias", "Bias", 2.0f),
            } };
        return info;
    }

    Fuzz() : DrivePedal (describe()) {}

private:
    void prepareTone() override
    {
        for (auto& c : channels)
        {
            c.preHp.set (35.0f, fs);
            c.post1.set (900.0f, fs);   // tone low-pass branch
            c.dc.set (1100.0f, fs);     // tone high-pass branch
            c.post2.set (8000.0f, fs);
        }
    }

    void updateFilters() override
    {
        offset = 0.05f + 0.06f * p (3);
        deadZone = 0.025f * p (3);
        tone = p (1) / 10.0f;
    }

    float driveTarget() const noexcept override { return dbToGain (16.0f + 3.2f * p (0)); }
    float levelTarget() const noexcept override { return dbToGain (-31.0f + 4.0f * p (2)); }
    float preShape (Channel& c, float x) noexcept override { return c.preHp.process (x); }

    float clip (float x) noexcept override
    {
        float y = transistor (x + offset) - transistor (offset);
        const float a = std::abs (y);
        y = a <= deadZone ? 0.0f : std::copysign ((a - deadZone) / (1.0f - deadZone), y);
        return softClip (4.0f * y) * 0.8f;
    }

    float postShape (Channel& c, float wet, float) noexcept override
    {
        const float lp = c.post1.process (wet) * 1.4f;
        const float hp = c.dc.process (wet) * 1.2f;
        return c.post2.process (lerp (lp, hp, tone));
    }

    static float transistor (float x) noexcept
    {
        const float a = std::abs (x);
        return x / std::cbrt (1.0f + a * a * a);
    }

    float offset = 0.1f, deadZone = 0.0f, tone = 0.5f;
};

} // namespace

void addDynamicsAndDriveEffects (std::vector<EffectEntry>& list)
{
    list.push_back ({ &Compressor::describe(), &makeEffect<Compressor> });
    list.push_back ({ &Boost::describe(), &makeEffect<Boost> });
    list.push_back ({ &Overdrive::describe(), &makeEffect<Overdrive> });
    list.push_back ({ &Distortion::describe(), &makeEffect<Distortion> });
    list.push_back ({ &Fuzz::describe(), &makeEffect<Fuzz> });
}

} // namespace oap::fx
