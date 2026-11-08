#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <random>
#include <vector>

//==============================================================================
/** Small signal generators for the DSP tests and the demo renders.
    The "guitar" is a Karplus-Strong plucked string - crude, but it has the
    transient/decay behaviour of a real DI signal, which a sine does not.
*/
namespace testsignals
{

struct PluckedString
{
    std::vector<float> line;
    size_t pos = 0;
    float decay = 0.996f, last = 0.0f;

    void pluck (double sampleRate, double hz, float amplitude, float brightness, float sustain, std::mt19937& rng)
    {
        const auto n = (size_t) juce::jmax (2.0, sampleRate / hz - 0.5);
        line.assign (n, 0.0f);
        std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
        float lp = 0.0f;
        for (auto& s : line)
        {
            lp += brightness * (dist (rng) - lp); // pick brightness
            s = lp * amplitude;
        }
        pos = 0;
        decay = sustain;
        last = 0.0f;
    }

    float next() noexcept
    {
        if (line.empty())
            return 0.0f;

        const size_t nextPos = (pos + 1) % line.size();
        const float out = line[pos];
        line[pos] = decay * 0.5f * (line[pos] + line[nextPos]);
        pos = nextPos;
        return out;
    }
};

struct NoteEvent
{
    double timeSeconds;
    std::vector<double> frequencies; // one entry per string
    float amplitude;
    float brightness;
    float sustain;
    double muteAfterSeconds; // 0 = let ring
};

inline double midiToHz (int note) { return 440.0 * std::pow (2.0, (note - 69) / 12.0); }

inline juce::AudioBuffer<float> renderGuitar (const std::vector<NoteEvent>& events, double sampleRate,
                                              double lengthSeconds, unsigned seed = 1)
{
    std::mt19937 rng (seed);
    const int numSamples = (int) (lengthSeconds * sampleRate);
    juce::AudioBuffer<float> out (1, numSamples);
    out.clear();

    std::vector<PluckedString> strings (6);

    for (size_t e = 0; e < events.size(); ++e)
    {
        const auto& ev = events[e];
        const int start = (int) (ev.timeSeconds * sampleRate);
        const int end = e + 1 < events.size() ? (int) (events[e + 1].timeSeconds * sampleRate) : numSamples;
        const int muteAt = ev.muteAfterSeconds > 0.0 ? start + (int) (ev.muteAfterSeconds * sampleRate) : end;

        for (size_t s = 0; s < ev.frequencies.size() && s < strings.size(); ++s)
            strings[s].pluck (sampleRate, ev.frequencies[s], ev.amplitude, ev.brightness, ev.sustain, rng);

        for (int i = start; i < end && i < numSamples; ++i)
        {
            float sum = 0.0f;
            for (size_t s = 0; s < ev.frequencies.size() && s < strings.size(); ++s)
            {
                if (i == muteAt)
                    strings[s].decay = 0.4f; // palm mute: decay is applied once per string period
                sum += strings[s].next();
            }
            out.addSample (0, i, sum);
        }
    }

    return out;
}

/** A mix of palm-muted chugs and ringing power chords, peaking around -8 dBFS like a hot DI. */
inline juce::AudioBuffer<float> heavyRiff (double sampleRate)
{
    const double e2 = midiToHz (40), b2 = midiToHz (47), e3 = midiToHz (52);
    const double g2 = midiToHz (43), d3 = midiToHz (50);
    const double a2 = midiToHz (45);
    std::vector<NoteEvent> ev;
    double t = 0.0;
    const double sixteenth = 60.0 / 140.0 / 4.0;

    for (int bar = 0; bar < 2; ++bar)
    {
        for (int i = 0; i < 6; ++i, t += sixteenth)
            ev.push_back ({ t, { e2, b2 }, 0.22f, 0.5f, 0.995f, sixteenth * 0.8 });
        ev.push_back ({ t, { g2, d3 }, 0.25f, 0.7f, 0.9985f, 0.0 }); t += sixteenth * 2;
        for (int i = 0; i < 4; ++i, t += sixteenth)
            ev.push_back ({ t, { e2, b2 }, 0.22f, 0.5f, 0.995f, sixteenth * 0.8 });
        ev.push_back ({ t, { a2, midiToHz (52) }, 0.25f, 0.7f, 0.9985f, 0.0 }); t += sixteenth * 4;
    }
    ev.push_back ({ t, { e2, b2, e3 }, 0.25f, 0.8f, 0.9992f, 0.0 });
    return renderGuitar (ev, sampleRate, t + 3.0, 7);
}

/** Arpeggiated clean chords. */
inline juce::AudioBuffer<float> cleanArpeggio (double sampleRate)
{
    const std::vector<std::vector<int>> chords {
        { 48, 52, 55, 59, 64 }, // Cmaj7
        { 45, 52, 55, 60, 64 }, // Am7
        { 41, 48, 53, 57, 64 }, // Fmaj7
        { 43, 50, 55, 59, 62 }, // G
    };

    std::vector<NoteEvent> ev;
    double t = 0.0;
    const double eighth = 60.0 / 92.0 / 2.0;
    for (const auto& c : chords)
    {
        for (int i = 0; i < 8; ++i, t += eighth)
        {
            const int idx = i < 5 ? i : 8 - i;
            ev.push_back ({ t, { midiToHz (c[(size_t) idx]) }, 0.28f, 0.45f, 0.9993f, 0.0 });
        }
    }
    ev.push_back ({ t, { midiToHz (48), midiToHz (52), midiToHz (55), midiToHz (60), midiToHz (64) }, 0.2f, 0.5f, 0.9995f, 0.0 });
    return renderGuitar (ev, sampleRate, t + 4.0, 3);
}

inline double rmsDb (const float* x, int n)
{
    double s = 0.0;
    for (int i = 0; i < n; ++i)
        s += (double) x[i] * x[i];
    return 10.0 * std::log10 (s / juce::jmax (1, n) + 1.0e-30);
}

inline double peakDb (const float* x, int n)
{
    float p = 0.0f;
    for (int i = 0; i < n; ++i)
        p = juce::jmax (p, std::abs (x[i]));
    return 20.0 * std::log10 (p + 1.0e-30);
}

} // namespace testsignals
