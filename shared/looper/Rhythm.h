#pragma once

#include <cstdint>

namespace oap::looper
{

//==============================================================================
/** The looper's guide rhythm: metronome clicks and simple drum grooves,
    synthesised on the fly (no samples). It follows the looper's beat grid, so
    it plays in time with the host or with the looper's own clock, in any time
    signature.

    Plain C++17. process() is real-time safe: no allocation, no locks. */
class RhythmGuide
{
public:
    /** Patterns 0..numClickPatterns-1 are clicks, the rest drum grooves (written in 4/4; in
        other time signatures they are cut at the bar line or repeated within it). */
    static constexpr int numPatterns = 14;
    static constexpr int numClickPatterns = 5;

    static const char* patternName (int pattern) noexcept;
    static bool isClick (int pattern) noexcept { return pattern < numClickPatterns; }

    void prepare (double sampleRate);

    /** Silences everything and forgets the position (the next step on the grid sounds). */
    void reset() noexcept;

    /** Where the bar is: quarter notes per bar and the time signature it comes from. */
    struct Meter
    {
        double quartersPerBar = 4.0;
        int numerator = 4, denominator = 4;
    };

    /** One output sample (mono). `grid` is the position in quarter notes (bar lines at multiples of
        quartersPerBar). `playPattern` plays the pattern, `countIn` plays count-in clicks on the
        beats; voices that are still ringing always finish. */
    float process (double grid, const Meter&, int pattern, bool playPattern, bool countIn, float level) noexcept;

    /** What a step of the grid holds, for tests: 12 steps per quarter note. */
    static constexpr int stepsPerQuarter = 12;

private:
    enum Instrument { kick, snare, closedHat, openHat, rim, clickAccent, clickPulse, clickBeat, clickSub, numInstruments };

    void triggerStep (std::int64_t step, const Meter&, int pattern, bool playPattern, bool countIn) noexcept;
    void clickStep (int stepInBar, const Meter&, int pattern, bool subdivisions) noexcept;
    void hit (Instrument, float velocity) noexcept;
    float noise() noexcept;

    double sampleRate = 48000.0;
    std::int64_t lastStep = 0;
    bool fresh = true;
    std::uint32_t seed = 0x1234567u;

    // decay factors per sample
    float kickDecay = 0.0f, kickPitchDecay = 0.0f, kickClickDecay = 0.0f, snareToneDecay = 0.0f, snareNoiseDecay = 0.0f,
          closedDecay = 0.0f, openDecay = 0.0f, rimDecay = 0.0f, rimNoiseDecay = 0.0f, clickDecay = 0.0f, clickAccentDecay = 0.0f,
          woodDecay = 0.0f;
    float hpSnare = 0.0f, lpSnare = 0.0f, hpHat = 0.0f, bpRim = 0.0f;
    float twoPiOverRate = 0.0f;

    // voices
    float kickAmp = 0.0f, kickPitch = 0.0f, kickClick = 0.0f, kickPhase = 0.0f;
    float snareTone = 0.0f, snareNoise = 0.0f, snarePhase = 0.0f, snareHpX = 0.0f, snareHpY = 0.0f, snareLp = 0.0f;
    float closedAmp = 0.0f, openAmp = 0.0f, hatX1 = 0.0f, hatY1 = 0.0f, hatX2 = 0.0f, hatY2 = 0.0f;
    float rimAmp = 0.0f, rimNoise = 0.0f, rimPhase = 0.0f, rimBp = 0.0f;
    float clickAmp = 0.0f, clickWood = 0.0f, clickPhase = 0.0f, clickPhase2 = 0.0f, clickFreq = 1000.0f, clickEnvDecay = 0.0f;
};

} // namespace oap::looper
