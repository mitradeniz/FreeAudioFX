#include "Rhythm.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace oap::looper
{

namespace
{
    constexpr float twoPi = 6.2831853071795864f;

    /** A drum groove, one bar of 4/4. Each track has one character per step:
        'X' accent, 'x' normal, 'o' soft (ghost note), anything else silent. */
    struct Groove
    {
        const char* name;
        int stepsPerQuarter; // 4 = sixteenth notes, 3 = eighth-note triplets
        const char* kick;
        const char* snare;
        const char* closedHat;
        const char* openHat;
        const char* rim;
    };

    constexpr Groove grooves[] = {
        //  name          grid  kick                  snare                 closed hat            open hat              rim
        { "Rock",          4, "x.......x.x.....",   "....x.......x...",   "x.x.x.x.x.x.x.x.",   "",                   "" },
        { "Pop",           4, "x.....x.x.......",   "....x.......x...",   "xoxoxoxoxoxoxoxo",   "",                   "" },
        { "Funk",          4, "x.x...x...x..x..",   "....X..o.o..X..o",   "xoxoxoxoxoxoxoxo",   "",                   "" },
        { "Ballad",        4, "x.....x.........",   "........x.......",   "x.x.x.x.x.x.x.x.",   "",                   "" },
        { "Disco",         4, "x...x...x...x...",   "....x.......x...",   "x...x...x...x...",   "..x...x...x...x.",   "" },
        { "Shuffle",       3, "x.....x.....",       "...x.....x..",       "x.xx.xx.xx.x",       "",                   "" },
        { "Reggae",        4, "........x.......",   "",                   "o.x.o.x.o.x.o.x.",   "",                   "........x......." },
        { "Metal",         4, "xoxoxoxoxoxoxoxo",   "....X.......X...",   "x...x...x...x...",   "x...............",   "" },
        { "Bossa Nova",    4, "x..xx..xx..xx..x",   "",                   "oooooooooooooooo",   "",                   "x..x..x...x..x.." },
    };

    constexpr const char* clickNames[RhythmGuide::numClickPatterns] = {
        "Click", "Click 8ths", "Click Triplets", "Click 16ths", "Click Shuffle"
    };

    static_assert (RhythmGuide::numPatterns == RhythmGuide::numClickPatterns + (int) (sizeof (grooves) / sizeof (grooves[0])),
                   "numPatterns must match the tables");

    float velocityOf (char c) noexcept
    {
        switch (c)
        {
            case 'X': return 1.0f;
            case 'x': return 0.8f;
            case 'o': return 0.4f;
            default:  return 0.0f;
        }
    }

    float decayFor (double seconds, double rate) noexcept
    {
        return (float) std::exp (-1.0 / (seconds * rate));
    }

    /** One-pole high-pass coefficient. */
    float highPassFor (double hz, double rate) noexcept
    {
        return (float) std::exp (-6.283185307179586 * std::min (hz, 0.49 * rate) / rate);
    }

    /** Steps of the grid in one note value of the time signature (12 per quarter note). */
    int unitSteps (int denominator) noexcept
    {
        switch (denominator)
        {
            case 1:  return 48;
            case 2:  return 24;
            case 8:  return 6;
            case 16: return 3;
            case 4:
            default: return 12;
        }
    }

    float flush (float x) noexcept { return std::abs (x) < 1.0e-6f ? 0.0f : x; }
}

//==============================================================================
const char* RhythmGuide::patternName (int pattern) noexcept
{
    if (pattern >= 0 && pattern < numClickPatterns)
        return clickNames[pattern];
    if (pattern >= numClickPatterns && pattern < numPatterns)
        return grooves[pattern - numClickPatterns].name;
    return "";
}

void RhythmGuide::prepare (double rate)
{
    sampleRate = std::max (8000.0, rate);
    twoPiOverRate = (float) (6.283185307179586 / sampleRate);

    kickDecay = decayFor (0.2, sampleRate);
    kickPitchDecay = decayFor (0.032, sampleRate);
    kickClickDecay = decayFor (0.002, sampleRate);
    snareToneDecay = decayFor (0.07, sampleRate);
    snareNoiseDecay = decayFor (0.13, sampleRate);
    closedDecay = decayFor (0.035, sampleRate);
    openDecay = decayFor (0.28, sampleRate);
    rimDecay = decayFor (0.012, sampleRate);
    rimNoiseDecay = decayFor (0.006, sampleRate);
    clickDecay = decayFor (0.014, sampleRate);
    clickAccentDecay = decayFor (0.02, sampleRate);
    woodDecay = decayFor (0.005, sampleRate);

    hpSnare = highPassFor (1500.0, sampleRate);
    lpSnare = 1.0f - (float) std::exp (-6.283185307179586 * std::min (8000.0, 0.45 * sampleRate) / sampleRate);
    hpHat = highPassFor (7000.0, sampleRate);
    bpRim = highPassFor (2500.0, sampleRate);

    reset();
}

void RhythmGuide::reset() noexcept
{
    kickAmp = kickPitch = kickClick = kickPhase = 0.0f;
    snareTone = snareNoise = snarePhase = snareHpX = snareHpY = snareLp = 0.0f;
    closedAmp = openAmp = hatX1 = hatY1 = hatX2 = hatY2 = 0.0f;
    rimAmp = rimNoise = rimPhase = rimBp = 0.0f;
    clickAmp = clickWood = clickPhase = clickPhase2 = 0.0f;
    fresh = true;
}

float RhythmGuide::noise() noexcept
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return (float) (std::int32_t) seed * (1.0f / 2147483648.0f);
}

//==============================================================================
void RhythmGuide::hit (Instrument instrument, float velocity) noexcept
{
    switch (instrument)
    {
        case kick:
            kickAmp = velocity;
            kickPitch = 1.0f;
            kickClick = velocity;
            kickPhase = 0.0f;
            break;

        case snare:
            snareTone = velocity;
            snareNoise = velocity;
            snarePhase = 0.0f;
            break;

        case closedHat:
            closedAmp = velocity;
            openAmp *= 0.15f; // a closing hi-hat chokes the open one
            break;

        case openHat:
            openAmp = velocity;
            break;

        case rim:
            rimAmp = velocity;
            rimNoise = velocity;
            rimPhase = 0.0f;
            break;

        case clickAccent:
        case clickPulse:
        case clickBeat:
        case clickSub:
        {
            static constexpr float freqs[] = { 1568.0f, 1174.7f, 1046.5f, 880.0f };
            static constexpr float gains[] = { 0.55f, 0.42f, 0.32f, 0.18f };
            const int k = (int) instrument - (int) clickAccent;
            clickFreq = std::min (freqs[k], 0.45f * (float) sampleRate);
            clickAmp = gains[k] * velocity;
            clickWood = 0.35f * clickAmp;
            clickPhase = clickPhase2 = 0.0f;
            clickEnvDecay = instrument == clickAccent ? clickAccentDecay : clickDecay;
            break;
        }

        case numInstruments:
        default:
            break;
    }
}

void RhythmGuide::clickStep (int p, const Meter& m, int pattern, bool subdivisions) noexcept
{
    if (p == 0)
    {
        hit (clickAccent, 1.0f);
        return;
    }

    const int unit = unitSteps (m.denominator);
    if (p % unit == 0)
    {
        // Notated beats. In compound time (6/8, 9/8, 12/8) they come in threes, and in other
        // x/8 bars in twos with a three at the end (7/8 = 2+2+3): the first of each group is stronger.
        const int beat = p / unit;
        bool groupStart = true;
        if (m.denominator >= 8 && m.numerator > 3)
        {
            if (m.numerator % 3 == 0)
                groupStart = beat % 3 == 0;
            else
                groupStart = beat < m.numerator - 3 ? beat % 2 == 0 : beat == m.numerator - 3;
        }
        hit (groupStart ? clickPulse : clickBeat, 1.0f);
        return;
    }

    if (! subdivisions)
        return;

    const int inQuarter = p % stepsPerQuarter;
    bool sub = false;
    switch (pattern)
    {
        case 1: sub = inQuarter % 6 == 0; break;   // eighths
        case 2: sub = inQuarter % 4 == 0; break;   // triplets
        case 3: sub = inQuarter % 3 == 0; break;   // sixteenths
        case 4: sub = inQuarter == 8; break;        // shuffle: the last triplet of each beat
        default: break;
    }

    if (sub)
        hit (clickSub, 1.0f);
}

void RhythmGuide::triggerStep (std::int64_t step, const Meter& m, int pattern, bool playPattern, bool countIn) noexcept
{
    const auto barSteps = std::max<std::int64_t> (1, std::llround (m.quartersPerBar * stepsPerQuarter));
    auto p = step % barSteps;
    if (p < 0)
        p += barSteps;

    const bool clickPattern = isClick (pattern);

    if (playPattern && clickPattern)
        clickStep ((int) p, m, pattern, true);
    else if (countIn)
        clickStep ((int) p, m, 0, false);

    if (! playPattern || clickPattern)
        return;

    const auto& groove = grooves[std::clamp (pattern - numClickPatterns, 0, numPatterns - numClickPatterns - 1)];
    const int res = stepsPerQuarter / groove.stepsPerQuarter;
    if (p % res != 0)
        return;

    const auto length = (std::int64_t) (4 * groove.stepsPerQuarter);
    const auto index = (size_t) ((p / res) % length);

    const auto play = [this, index] (const char* track, Instrument instrument)
    {
        if (std::strlen (track) > index)
            if (const float v = velocityOf (track[index]); v > 0.0f)
                hit (instrument, v);
    };

    play (groove.kick, kick);
    play (groove.snare, snare);
    play (groove.closedHat, closedHat);
    play (groove.openHat, openHat);
    play (groove.rim, rim);
}

//==============================================================================
float RhythmGuide::process (double grid, const Meter& m, int pattern, bool playPattern, bool countIn, float level) noexcept
{
    // --- the grid: one step every 1/12 quarter note
    const double scaled = grid * stepsPerQuarter + 1.0e-6;
    const auto step = (std::int64_t) std::floor (scaled);

    if (fresh || step != lastStep)
    {
        // Normally the grid moves on by one step. After a jump (the host relocated, the clock was
        // restarted), only a step that has just begun is played.
        const bool next = ! fresh && step == lastStep + 1;
        if (next || scaled - (double) step < 0.25)
            triggerStep (step, m, pattern, playPattern, countIn);
        lastStep = step;
        fresh = false;
    }

    // --- voices
    float drums = 0.0f, out = 0.0f;

    if (kickAmp > 0.0f || kickClick > 0.0f)
    {
        const float freq = 48.0f + 115.0f * kickPitch;
        kickPhase += freq * twoPiOverRate;
        if (kickPhase > twoPi)
            kickPhase -= twoPi;
        drums += 0.9f * kickAmp * std::sin (kickPhase) + 0.3f * kickClick * noise();
        kickAmp = flush (kickAmp * kickDecay);
        kickPitch *= kickPitchDecay;
        kickClick = flush (kickClick * kickClickDecay);
    }

    if (snareTone > 0.0f || snareNoise > 0.0f)
    {
        snarePhase += 190.0f * twoPiOverRate;
        if (snarePhase > twoPi)
            snarePhase -= twoPi;

        const float x = noise();
        snareHpY = hpSnare * (snareHpY + x - snareHpX);
        snareHpX = x;
        snareLp += lpSnare * (snareHpY - snareLp);

        drums += 0.35f * snareTone * std::sin (snarePhase) + 0.75f * snareNoise * snareLp;
        snareTone = flush (snareTone * snareToneDecay);
        snareNoise = flush (snareNoise * snareNoiseDecay);
    }

    if (closedAmp > 0.0f || openAmp > 0.0f)
    {
        const float x = noise();
        hatY1 = hpHat * (hatY1 + x - hatX1);
        hatX1 = x;
        hatY2 = hpHat * (hatY2 + hatY1 - hatX2);
        hatX2 = hatY1;

        drums += hatY2 * (0.4f * closedAmp + 0.3f * openAmp);
        closedAmp = flush (closedAmp * closedDecay);
        openAmp = flush (openAmp * openDecay);
    }

    if (rimAmp > 0.0f || rimNoise > 0.0f)
    {
        rimPhase += std::min (1700.0f, 0.45f * (float) sampleRate) * twoPiOverRate;
        if (rimPhase > twoPi)
            rimPhase -= twoPi;

        const float x = noise();
        rimBp = bpRim * rimBp + (1.0f - bpRim) * x;

        drums += 0.4f * rimAmp * std::sin (rimPhase) + 0.5f * rimNoise * (x - rimBp);
        rimAmp = flush (rimAmp * rimDecay);
        rimNoise = flush (rimNoise * rimNoiseDecay);
    }

    if (clickAmp > 0.0f || clickWood > 0.0f)
    {
        clickPhase += clickFreq * twoPiOverRate;
        if (clickPhase > twoPi)
            clickPhase -= twoPi;
        clickPhase2 += std::min (2.76f * clickFreq, 0.45f * (float) sampleRate) * twoPiOverRate;
        if (clickPhase2 > twoPi)
            clickPhase2 -= twoPi;

        out += clickAmp * std::sin (clickPhase) + clickWood * std::sin (clickPhase2);
        clickAmp = flush (clickAmp * clickEnvDecay);
        clickWood = flush (clickWood * woodDecay);
    }

    // The drums are a guide under your playing, about -20 dB RMS at full level; the clicks cut through.
    return (0.5f * drums + out) * std::clamp (level, 0.0f, 1.0f);
}

} // namespace oap::looper
