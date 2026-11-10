// The effect amps. Each entry is a complete amp: voicing, built-in effects,
// extra faceplate controls and colours.

#include "AmpModel.h"

namespace oap::amps
{

namespace
{
    using dsp::CabinetSection;
    using dsp::ToneStack;
    using fx::ParamSpec;

    AmpModel dream()
    {
        AmpModel m;
        m.id = "dream"; m.titleAccent = "DREAM"; m.titleRest = "AMP"; m.name = "Dream Amp";
        m.tagline = "CLEAN AMP  /  CHORUS  /  ECHO  /  SHIMMER";
        m.description = "A glassy clean amp that floats in a wide chorus, ping-pong echoes and an octave-up shimmer reverb. Instant dream pop and ambient swells.";
        m.look = { 0xff1b1730, 0xff3d3660, 0xff242040, 0xffb49bff, false };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f; // clean amps get loud fast: keep the top of Master usable
        v.gainCompensation = 0.3; v.masterCompensation = 0.5; v.outputTrim = 0.56;
        v.inputHighpassHz = 40.0; v.brightHz = 2500.0; v.brightDb = 3.0;
        v.stages = { { 2.0, 0.10, 20.0, 18000.0 }, { 3.0, 0.18, 25.0, 12000.0 } };
        v.gainMinDb = -26.0f; v.gainMaxDb = 10.0f;
        v.toneStack = ToneStack::blackface();
        v.sag = 0.6; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 4.0f; m.defaultTreble = 6.5f;

        m.effects = { { "chorus", false, { { "mode", 2.0f }, { "tone", 7.0f } } },
                      { "delay", false, { { "mode", 2.0f }, { "tone", 5.0f }, { "time", 420.0f } } },
                      { "shimmer", false, { { "interval", 0.0f }, { "tone", 6.0f } } } };
        m.fxControls = { ParamSpec::knob ("chorus", "Chorus", 4.0f), ParamSpec::knob ("echo", "Echo", 4.0f),
                         ParamSpec::knob ("shimmer", "Shimmer", 5.0f), ParamSpec::knob ("space", "Space", 6.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "mix", k[0] * 7.0f);
            fx.set (0, "depth", 3.0f + k[0] * 0.5f);
            fx.set (0, "rate", 0.25f + k[0] * 0.05f);
            fx.set (1, "mix", k[1] * 4.5f);
            fx.set (1, "feedback", 20.0f + k[1] * 4.0f);
            fx.set (2, "shimmer", k[2] * 9.0f);
            fx.set (2, "decay", k[3]);
            fx.set (2, "mix", 15.0f + k[3] * 5.0f);
        };
        return m;
    }

    AmpModel dub()
    {
        AmpModel m;
        m.id = "dub"; m.titleAccent = "DUB"; m.titleRest = "AMP"; m.name = "Dub Amp";
        m.tagline = "REGGAE CLEAN  /  TAPE DUB ECHO  /  SPRING";
        m.description = "A tight, bright clean amp for reggae skanks with a dub tape echo you can push into endless repeats and a short spring reverb.";
        m.look = { 0xff13221a, 0xff2f6b3a, 0xff1d4325, 0xffffd23f, false };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f; // clean amps get loud fast: keep the top of Master usable
        v.gainCompensation = 0.3; v.masterCompensation = 0.5; v.outputTrim = 0.31;
        v.inputHighpassHz = 140.0; v.brightHz = 2000.0; v.brightDb = 2.0;
        v.preMidHz = 1200.0; v.preMidQ = 0.8; v.preMidDb = 4.0;
        v.stages = { { 2.5, 0.12, 40.0, 14000.0 }, { 3.5, 0.20, 60.0, 10000.0 } };
        v.gainMinDb = -26.0f; v.gainMaxDb = 8.0f;
        v.toneStack = ToneStack::bassman();
        v.sag = 0.3; v.defaultCabinet = CabinetSection::Combo1x12;
        m.defaultGain = 4.0f; m.defaultBass = 3.0f; m.defaultMiddle = 6.0f; m.defaultTreble = 6.5f;

        m.effects = { { "tapeecho", false, { { "heads", 0.0f }, { "wobble", 3.0f }, { "age", 5.0f } } },
                      { "reverb", false, { { "type", 3.0f }, { "predelay", 0.0f }, { "tone", 5.0f } } } };
        m.fxControls = { ParamSpec::range ("time", "Echo Time", 120.0f, 700.0f, 375.0f, "ms", 0, 350.0f),
                         ParamSpec::knob ("feedback", "Feedback", 5.0f), ParamSpec::knob ("echo", "Echo", 4.0f),
                         ParamSpec::knob ("spring", "Spring", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "rate", k[0]);
            fx.set (0, "intensity", 1.0f + k[1] * 0.85f);
            fx.set (0, "echo", k[2] * 8.0f);
            fx.set (1, "mix", k[3] * 6.0f);
            fx.set (1, "decay", 3.0f + k[3] * 0.4f);
        };
        return m;
    }

    AmpModel spaceDelay()
    {
        AmpModel m;
        m.id = "spacedelay"; m.titleAccent = "SPACE"; m.titleRest = "DELAY AMP"; m.name = "Space Delay Amp";
        m.tagline = "WARM CRUNCH  /  3-HEAD TAPE ECHO  /  SPRING";
        m.description = "A warm crunch amp with a three-head tape echo and spring reverb built in, from slapback to self-oscillating space trips.";
        m.look = { 0xff1b1f24, 0xffcfd3d8, 0xff9ea4aa, 0xff2f7fd6, true };

        auto& v = m.voicing;
        v.outputTrim = 0.33;
        v.inputHighpassHz = 50.0; v.brightHz = 1800.0; v.brightDb = 2.0;
        v.stages = { { 5.0, 0.15, 30.0, 15000.0 }, { 8.0, 0.30, 80.0, 9000.0 }, { 1.5, 0.20, 40.0, 8000.0 } };
        v.gainMinDb = -34.0f; v.gainMaxDb = 4.0f;
        v.toneStack = ToneStack::bassman();
        v.sag = 0.5; v.defaultCabinet = CabinetSection::Open2x12;

        m.effects = { { "tapeecho", false, { { "wobble", 4.0f }, { "age", 4.0f } } },
                      { "reverb", false, { { "type", 3.0f }, { "tone", 5.0f }, { "decay", 4.0f } } } };
        m.fxControls = { ParamSpec::range ("rate", "Repeat Rate", 50.0f, 700.0f, 300.0f, "ms", 0, 250.0f),
                         ParamSpec::knob ("intensity", "Intensity", 4.5f), ParamSpec::knob ("echo", "Echo", 4.0f),
                         ParamSpec::choice ("heads", "Heads", { "1", "2", "3", "1+2", "2+3", "1+2+3" }, 3),
                         ParamSpec::knob ("reverb", "Reverb", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "rate", k[0]);
            fx.set (0, "intensity", k[1]);
            fx.set (0, "echo", k[2] * 10.0f);
            fx.set (0, "heads", k[3]);
            fx.set (1, "mix", k[4] * 5.0f);
        };
        return m;
    }

    AmpModel blackMetal()
    {
        AmpModel m;
        m.id = "blackmetal"; m.titleAccent = "BLACK"; m.titleRest = "METAL AMP"; m.name = "Black Metal Amp";
        m.tagline = "RAW BUZZSAW GAIN  /  COLD HALL REVERB";
        m.description = "Thin, cold and razor sharp: a trebly buzzsaw high-gain amp with scooped mids, a fast gate and a dark cathedral hall for tremolo-picked riffs.";
        m.look = { 0xff0a0a0b, 0xff1d1d21, 0xff0e0e10, 0xff9fc3d9, false };

        auto& v = m.voicing;
        v.outputTrim = 0.24;
        v.inputHighpassHz = 150.0; v.brightHz = 2400.0; v.brightDb = 6.0;
        v.stages = { { 7.0, 0.12, 40.0, 16000.0 }, { 18.0, 0.40, 140.0, 12000.0 },
                     { 2.2, 0.20, 90.0, 11000.0 }, { 1.3, 0.08, 40.0, 10000.0 } };
        v.gainMinDb = -34.0f; v.gainMaxDb = 6.0f;
        v.toneStack = ToneStack::british();
        v.postMidHz = 650.0; v.postMidQ = 0.9; v.postMidDb = -6.0;
        v.presenceDb = 4.0; v.powerBandwidthHz = 13500.0;
        v.sag = 0.15; v.oversamplingOrder = 3;
        v.hasGate = true; v.gateDefaultDb = -60.0f;
        m.defaultGain = 7.5f; m.defaultBass = 4.0f; m.defaultMiddle = 3.5f; m.defaultTreble = 7.0f;

        m.effects = { { "reverb", false, { { "type", 1.0f }, { "tone", 7.0f }, { "predelay", 30.0f } } } };
        m.fxControls = { ParamSpec::knob ("hall", "Hall", 4.0f), ParamSpec::knob ("decay", "Decay", 7.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "mix", k[0] * 6.0f);
            fx.set (0, "decay", k[1]);
        };
        return m;
    }

    AmpModel warmRock()
    {
        AmpModel m;
        m.id = "warmrock"; m.titleAccent = "WARM"; m.titleRest = "ROCK AMP"; m.name = "Warm Rock Amp";
        m.tagline = "BRITISH CRUNCH  /  ANALOG DELAY  /  PLATE";
        m.description = "A warm British-style crunch amp, from AC/DC-style rhythm to singing classic-rock leads, with an analog-style delay and a studio plate reverb.";
        m.look = { 0xff24180f, 0xffd9b25e, 0xffa8823a, 0xffd2452b, true };

        auto& v = m.voicing;
        v.outputTrim = 0.28;
        v.inputHighpassHz = 40.0; v.brightHz = 1600.0; v.brightDb = 2.5;
        v.stages = { { 6.0, 0.15, 30.0, 14000.0 }, { 10.0, 0.35, 90.0, 9000.0 }, { 2.0, 0.20, 60.0, 7500.0 } };
        v.gainMinDb = -36.0f; v.gainMaxDb = 4.0f;
        v.toneStack = ToneStack::british();
        v.presenceDb = 1.0; v.sag = 0.45;
        m.defaultGain = 6.0f; m.defaultMiddle = 6.5f;

        m.effects = { { "delay", false, { { "mode", 1.0f }, { "tone", 4.0f } } },
                      { "reverb", false, { { "type", 2.0f }, { "tone", 6.0f }, { "predelay", 10.0f }, { "decay", 4.5f } } } };
        m.fxControls = { ParamSpec::knob ("delay", "Delay", 3.0f),
                         ParamSpec::range ("time", "Delay Time", 80.0f, 900.0f, 380.0f, "ms", 0, 350.0f),
                         ParamSpec::knob ("plate", "Plate", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "mix", k[0] * 4.5f);
            fx.set (0, "feedback", 15.0f + k[0] * 4.0f);
            fx.set (0, "time", k[1]);
            fx.set (1, "mix", k[2] * 5.0f);
        };
        return m;
    }

    AmpModel surf()
    {
        AmpModel m;
        m.id = "surf"; m.titleAccent = "SURF"; m.titleRest = "AMP"; m.name = "Surf Amp";
        m.tagline = "BRIGHT CLEAN  /  DRIPPY SPRING TANK  /  TREMOLO";
        m.description = "Sparkling clean tone drenched in a drippy spring reverb tank, with an amp-style tremolo. Made for twangy instrumentals.";
        m.look = { 0xff1c2a2e, 0xff8fd0c3, 0xff5fa699, 0xffe8572c, true };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f; // clean amps get loud fast: keep the top of Master usable
        v.gainCompensation = 0.3; v.masterCompensation = 0.5; v.outputTrim = 0.40;
        v.inputHighpassHz = 60.0; v.brightHz = 2200.0; v.brightDb = 5.0;
        v.stages = { { 2.2, 0.10, 25.0, 18000.0 }, { 3.0, 0.15, 30.0, 13000.0 } };
        v.gainMinDb = -24.0f; v.gainMaxDb = 10.0f;
        v.toneStack = ToneStack::blackface();
        v.sag = 0.7; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 4.5f; m.defaultBass = 4.0f; m.defaultTreble = 7.0f;

        m.effects = { { "tremolo", false, { { "shape", 3.0f }, { "mode", 0.0f } } },
                      { "reverb", false, { { "type", 3.0f }, { "tone", 6.0f } } } };
        m.fxControls = { ParamSpec::knob ("dwell", "Dwell", 6.0f), ParamSpec::knob ("reverb", "Reverb", 6.0f),
                         ParamSpec::range ("speed", "Speed", 1.0f, 12.0f, 5.0f, "Hz", 1, 4.0f),
                         ParamSpec::knob ("intensity", "Intensity", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (1, "decay", 2.0f + k[0] * 0.8f);
            fx.set (1, "mix", k[1] * 8.0f);
            fx.set (0, "rate", k[2]);
            fx.set (0, "depth", k[3] * 10.0f);
        };
        return m;
    }

    AmpModel shoegaze()
    {
        AmpModel m;
        m.id = "shoegaze"; m.titleAccent = "SHOEGAZE"; m.titleRest = "AMP"; m.name = "Shoegaze Amp";
        m.tagline = "FUZZ WALL  /  VIBRATO WARBLE  /  ENDLESS WASH";
        m.description = "A wall of fuzz into a cranked amp, warbling vibrato and a huge washed-out hall: shoegaze and noise-pop in one box.";
        m.look = { 0xff241a26, 0xffe8a9c9, 0xffb97b9e, 0xff9d4edd, true };

        auto& v = m.voicing;
        v.outputTrim = 0.28;
        v.inputHighpassHz = 40.0;
        v.stages = { { 4.0, 0.15, 30.0, 12000.0 }, { 6.0, 0.30, 60.0, 8000.0 }, { 1.5, 0.20, 40.0, 7000.0 } };
        v.gainMinDb = -30.0f; v.gainMaxDb = 4.0f;
        v.toneStack = ToneStack::bassman();
        v.sag = 0.6; v.defaultCabinet = CabinetSection::Open2x12;

        m.effects = { { "fuzz", true, { { "tone", 6.0f }, { "bias", 1.0f } } },
                      { "chorus", false, { { "mode", 1.0f }, { "tone", 6.0f }, { "rate", 4.0f } } },
                      { "reverb", false, { { "type", 1.0f }, { "tone", 5.0f }, { "predelay", 40.0f } } } };
        m.fxControls = { ParamSpec::knob ("fuzz", "Fuzz", 6.0f), ParamSpec::knob ("warble", "Warble", 4.0f),
                         ParamSpec::knob ("wash", "Wash", 6.0f), ParamSpec::knob ("size", "Size", 7.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "fuzz", k[0]);
            fx.set (0, "volume", 6.0f - k[0] * 0.25f);
            fx.set (1, "depth", k[1]);
            fx.set (2, "mix", k[2] * 8.0f);
            fx.set (2, "decay", k[3]);
        };
        return m;
    }

    AmpModel arena()
    {
        AmpModel m;
        m.id = "arena"; m.titleAccent = "ARENA"; m.titleRest = "AMP"; m.name = "Arena Amp";
        m.tagline = "80S HOT-ROD LEAD  /  STEREO CHORUS  /  PING-PONG  /  PLATE";
        m.description = "The big 80s rack sound: a hot-rodded lead amp into stereo chorus, ping-pong delay and plate reverb. Stadium solos included.";
        m.look = { 0xff101015, 0xff2b2b35, 0xff15151b, 0xffff2fa6, false };

        auto& v = m.voicing;
        v.outputTrim = 0.23;
        v.inputHighpassHz = 70.0; v.brightHz = 1800.0; v.brightDb = 3.0;
        v.stages = { { 6.0, 0.15, 30.0, 14000.0 }, { 16.0, 0.45, 110.0, 9000.0 },
                     { 2.0, 0.25, 70.0, 7500.0 }, { 1.2, 0.10, 35.0, 6500.0 } };
        v.gainMinDb = -34.0f; v.gainMaxDb = 2.0f;
        v.toneStack = ToneStack::british();
        v.presenceDb = 3.0; v.sag = 0.25; v.oversamplingOrder = 3;
        v.hasGate = true; v.gateDefaultDb = -65.0f;
        m.defaultGain = 6.0f; m.defaultMiddle = 6.0f; m.defaultTreble = 6.5f;

        m.effects = { { "chorus", false, { { "mode", 2.0f }, { "rate", 0.6f }, { "tone", 7.0f } } },
                      { "delay", false, { { "mode", 2.0f }, { "time", 430.0f }, { "tone", 6.0f } } },
                      { "reverb", false, { { "type", 2.0f }, { "tone", 7.0f }, { "decay", 5.5f } } } };
        m.fxControls = { ParamSpec::knob ("chorus", "Chorus", 3.0f), ParamSpec::knob ("delay", "Delay", 4.0f),
                         ParamSpec::knob ("reverb", "Reverb", 4.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "mix", k[0] * 6.0f);
            fx.set (0, "depth", 3.0f + k[0] * 0.4f);
            fx.set (1, "mix", k[1] * 4.5f);
            fx.set (1, "feedback", 20.0f + k[1] * 4.0f);
            fx.set (2, "mix", k[2] * 5.0f);
        };
        return m;
    }

    AmpModel stereoJazz()
    {
        AmpModel m;
        m.id = "stereojazz"; m.titleAccent = "STEREO"; m.titleRest = "JAZZ AMP"; m.name = "Stereo Jazz Amp";
        m.tagline = "SOLID-STATE CLEAN  /  STEREO CHORUS  /  VIBRATO  /  SPRING";
        m.description = "A pristine solid-state clean amp with a lush built-in stereo chorus and vibrato. Jazz chords, funk and new-wave cleans.";
        m.look = { 0xff151515, 0xff3a3a3d, 0xff232325, 0xffc9d2db, false };

        auto& v = m.voicing;
        v.masterMaxDb = 3.0f; // solid state: no power amp compression, keep the top of Master usable
        v.gainCompensation = 0.5; v.masterCompensation = 0.5; v.outputTrim = 1.14;
        v.inputHighpassHz = 30.0; v.brightHz = 3000.0; v.brightDb = 2.0;
        v.stages = { { 1.6, 0.0, 15.0, 20000.0 }, { 2.0, 0.0, 15.0, 16000.0 } };
        v.gainMinDb = -24.0f; v.gainMaxDb = 6.0f;
        v.toneStack = ToneStack::bassman(); v.toneMakeup = 3.5;
        v.sag = 0.0; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 4.0f;

        m.effects = { { "chorus", false, { { "tone", 8.0f }, { "mix", 50.0f } } },
                      { "reverb", false, { { "type", 3.0f }, { "tone", 6.0f }, { "decay", 4.0f } } } };
        m.fxControls = { ParamSpec::range ("speed", "Speed", 0.1f, 6.0f, 0.6f, "Hz", 2, 1.0f),
                         ParamSpec::knob ("depth", "Depth", 6.0f), ParamSpec::toggle ("vibrato", "Vibrato", false),
                         ParamSpec::knob ("reverb", "Reverb", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "rate", k[0]);
            fx.set (0, "depth", k[1]);
            fx.set (0, "mode", k[2] > 0.5f ? 1.0f : 2.0f);
            fx.set (0, "mix", k[1] > 0.05f ? 50.0f : 0.0f);
            fx.set (1, "mix", k[3] * 5.0f);
        };
        return m;
    }

    AmpModel doom()
    {
        AmpModel m;
        m.id = "doom"; m.titleAccent = "DOOM"; m.titleRest = "AMP"; m.name = "Doom Amp";
        m.tagline = "FUZZ INTO A CRANKED AMP  /  SLOW PHASER  /  ROOM";
        m.description = "Low and slow: a thick fuzz into a cranked, saggy amp with a huge low end, a slow phaser and a dark room. Stoner, doom and sludge.";
        m.look = { 0xff120c08, 0xff4c3222, 0xff2a1a10, 0xffff7a1a, false };

        auto& v = m.voicing;
        v.outputTrim = 0.37;
        v.inputHighpassHz = 25.0;
        v.stages = { { 5.0, 0.20, 20.0, 9000.0 }, { 8.0, 0.35, 40.0, 6000.0 }, { 1.6, 0.20, 30.0, 5500.0 } };
        v.gainMinDb = -30.0f; v.gainMaxDb = 6.0f;
        v.toneStack = ToneStack::british();
        v.lowShelfHz = 90.0; v.lowShelfDb = 5.0;
        v.postMidHz = 400.0; v.postMidQ = 1.0; v.postMidDb = 2.0;
        v.powerBandwidthHz = 7000.0; v.sag = 0.8;
        v.hasGate = true; v.gateDefaultDb = -62.0f;
        m.defaultGain = 6.0f; m.defaultBass = 7.0f; m.defaultTreble = 4.0f;

        m.effects = { { "fuzz", true, { { "tone", 3.0f }, { "bias", 3.0f } } },
                      { "phaser", false, { { "stages", 1.0f }, { "feedback", 50.0f }, { "depth", 7.0f } } },
                      { "reverb", false, { { "type", 0.0f }, { "tone", 4.0f }, { "decay", 4.0f } } } };
        m.fxControls = { ParamSpec::knob ("fuzz", "Fuzz", 7.0f), ParamSpec::knob ("phase", "Phase", 3.0f),
                         ParamSpec::range ("speed", "Phase Speed", 0.05f, 2.0f, 0.2f, "Hz", 2, 0.3f),
                         ParamSpec::knob ("room", "Room", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "fuzz", k[0]);
            fx.set (0, "volume", 6.0f - k[0] * 0.25f);
            fx.set (1, "mix", k[1] * 5.0f);
            fx.set (1, "rate", k[2]);
            fx.set (2, "mix", k[3] * 5.0f);
        };
        return m;
    }

    //==============================================================================
    AmpModel crunch()
    {
        AmpModel m;
        m.id = "crunch"; m.titleAccent = "CRUNCH"; m.titleRest = "AMP"; m.name = "Crunch Amp";
        m.tagline = "PLEXI-STYLE CRUNCH  /  TREBLE BOOSTER  /  ROOM";
        m.description = "A loud, open crunch amp in the spirit of late-60s plexi heads, with a treble booster in front for extra bite and a small room around it. Rock'n'roll, blues rock and punk.";
        m.look = { 0xff1b0f14, 0xff4a1e2a, 0xff2a0f18, 0xffe8c26a, false };

        auto& v = m.voicing;
        v.outputTrim = 0.30;
        v.inputHighpassHz = 40.0; v.brightHz = 2000.0; v.brightDb = 3.0;
        v.stages = { { 5.0, 0.15, 25.0, 15000.0 }, { 9.0, 0.32, 70.0, 9500.0 }, { 1.8, 0.20, 50.0, 8000.0 } };
        v.gainMinDb = -34.0f; v.gainMaxDb = 6.0f;
        v.toneStack = ToneStack::british();
        v.presenceDb = 2.0; v.sag = 0.5;
        m.defaultGain = 5.5f; m.defaultMiddle = 6.0f; m.defaultTreble = 6.5f;

        m.effects = { { "boost", true, { { "bass", -4.0f }, { "push", 3.0f } } },
                      { "reverb", false, { { "type", 0.0f }, { "tone", 6.0f }, { "decay", 3.5f }, { "predelay", 8.0f } } } };
        m.fxControls = { ParamSpec::knob ("booster", "Booster", 3.0f), ParamSpec::knob ("bite", "Bite", 5.0f),
                         ParamSpec::knob ("room", "Room", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "gain", k[0] * 1.8f);
            fx.set (0, "treble", -2.0f + k[1] * 1.2f);
            fx.set (1, "mix", k[2] * 5.0f);
        };
        return m;
    }

    AmpModel eightBit()
    {
        AmpModel m;
        m.id = "eightbit"; m.titleAccent = "8-BIT"; m.titleRest = "AMP"; m.name = "8-Bit Amp";
        m.tagline = "CHIPTUNE SYNTH  /  BIT CRUSHER  /  PIXEL ECHO";
        m.description = "Turns your guitar into a retro game console: a square-wave synth with a sub octave, a crunchy little amp, a bit crusher and a pixel-perfect echo. Chiptune, game-music covers and lo-fi beats.";
        m.look = { 0xff10142a, 0xff23305e, 0xff151d40, 0xff5df2a0, false };

        auto& v = m.voicing;
        v.gainCompensation = 0.3; v.outputTrim = 0.5;
        v.inputHighpassHz = 60.0; v.brightHz = 2500.0; v.brightDb = 2.0;
        v.stages = { { 3.0, 0.10, 30.0, 12000.0 }, { 4.0, 0.25, 60.0, 9000.0 } };
        v.gainMinDb = -26.0f; v.gainMaxDb = 8.0f;
        v.toneStack = ToneStack::bassman();
        v.sag = 0.3; v.defaultCabinet = CabinetSection::Combo1x12;
        m.defaultGain = 4.5f;

        m.effects = { { "synth", true, { { "filter", 7.0f }, { "resonance", 3.0f }, { "sweep", 4.0f } } },
                      { "lofi", false, { { "wobble", 1.0f }, { "filter", 7.0f }, { "drive", 2.0f }, { "mix", 100.0f } } },
                      { "delay", false, { { "mode", 0.0f }, { "time", 250.0f }, { "tone", 8.0f }, { "feedback", 30.0f } } } };
        m.fxControls = { ParamSpec::knob ("chip", "Chip", 6.0f), ParamSpec::range ("bits", "Bits", 4.0f, 16.0f, 8.0f, "", 0),
                         ParamSpec::knob ("crush", "Crush", 4.0f), ParamSpec::knob ("echo", "Echo", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "square", k[0] * 10.0f);
            fx.set (0, "sub", k[0] * 5.0f);
            fx.set (0, "dry", 100.0f - k[0] * 7.0f);
            fx.set (1, "bits", k[1]);
            fx.set (1, "rate", k[2]);
            fx.set (2, "mix", k[3] * 5.0f);
        };
        return m;
    }

    AmpModel knight()
    {
        AmpModel m;
        m.id = "knight"; m.titleAccent = "KNIGHT"; m.titleRest = "AMP"; m.name = "Knight Amp";
        m.tagline = "HEROIC LEAD  /  HARMONY  /  DRAGON ECHO  /  GREAT HALL";
        m.description = "A heroic, singing lead amp for power metal and epic rock, with a built-in harmony voice, a sweeping ping-pong echo and a great-hall reverb.";
        m.look = { 0xff10141f, 0xff2a3550, 0xff161d30, 0xffd9b45a, false };

        auto& v = m.voicing;
        v.outputTrim = 0.23;
        v.inputHighpassHz = 70.0; v.brightHz = 2200.0; v.brightDb = 3.0;
        v.stages = { { 6.0, 0.15, 30.0, 14000.0 }, { 14.0, 0.42, 100.0, 9500.0 },
                     { 2.0, 0.25, 70.0, 8000.0 }, { 1.2, 0.10, 35.0, 7000.0 } };
        v.gainMinDb = -34.0f; v.gainMaxDb = 4.0f;
        v.toneStack = ToneStack::british();
        v.postMidHz = 800.0; v.postMidQ = 0.8; v.postMidDb = 2.5;
        v.presenceDb = 3.0; v.sag = 0.3; v.oversamplingOrder = 3;
        v.hasGate = true; v.gateDefaultDb = -65.0f;
        m.defaultGain = 6.5f; m.defaultMiddle = 6.5f; m.defaultTreble = 6.5f;

        m.effects = { { "pitch", false, { { "interval", 7.0f }, { "fine", 0.0f }, { "bend", 100.0f }, { "tone", 6.0f } } },
                      { "delay", false, { { "mode", 2.0f }, { "time", 400.0f }, { "tone", 6.0f } } },
                      { "reverb", false, { { "type", 1.0f }, { "tone", 6.0f }, { "predelay", 25.0f } } } };
        m.fxControls = { ParamSpec::knob ("harmony", "Harmony", 3.0f),
                         ParamSpec::choice ("interval", "Interval", { "Min 3rd", "Maj 3rd", "4th", "5th", "Octave" }, 3),
                         ParamSpec::knob ("echo", "Echo", 4.0f), ParamSpec::knob ("hall", "Hall", 4.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            static constexpr float intervals[] = { 3.0f, 4.0f, 5.0f, 7.0f, 12.0f };
            fx.set (0, "mix", k[0] * 5.0f);
            fx.set (0, "interval", intervals[std::clamp ((int) (k[1] + 0.5f), 0, 4)]);
            fx.set (1, "mix", k[2] * 4.5f);
            fx.set (1, "feedback", 20.0f + k[2] * 4.0f);
            fx.set (2, "mix", k[3] * 5.0f);
            fx.set (2, "decay", 4.0f + k[3] * 0.4f);
        };
        return m;
    }

    AmpModel heavyMetal()
    {
        AmpModel m;
        m.id = "heavymetal"; m.titleAccent = "HEAVY"; m.titleRest = "METAL AMP"; m.name = "Heavy Metal Amp";
        m.tagline = "80S HEAVY METAL  /  TIGHT BOOST  /  DELAY  /  PLATE";
        m.description = "Leather, chrome and lightning: a tight, mid-forward 80s heavy metal amp with a gate, a tight boost in front, a solo delay and a plate reverb. Galloping riffs and screaming solos.";
        m.look = { 0xff0b0c10, 0xff2a2d35, 0xff121318, 0xff5cc8ff, false };

        auto& v = m.voicing;
        v.outputTrim = 0.23;
        v.inputHighpassHz = 80.0; v.brightHz = 2000.0; v.brightDb = 2.0;
        v.stages = { { 6.0, 0.15, 30.0, 14000.0 }, { 16.0, 0.45, 110.0, 9000.0 },
                     { 2.0, 0.25, 70.0, 7500.0 }, { 1.2, 0.10, 35.0, 6500.0 } };
        v.gainMinDb = -34.0f; v.gainMaxDb = 4.0f;
        v.toneStack = ToneStack::british();
        v.postMidHz = 750.0; v.postMidQ = 0.9; v.postMidDb = 3.0;
        v.presenceDb = 2.0; v.sag = 0.25; v.oversamplingOrder = 3;
        v.hasGate = true; v.gateDefaultDb = -62.0f;
        m.defaultGain = 6.5f; m.defaultMiddle = 6.5f; m.defaultTreble = 6.5f;

        m.effects = { { "boost", true, { { "treble", 1.0f }, { "push", 2.0f } } },
                      { "delay", false, { { "mode", 0.0f }, { "time", 420.0f }, { "tone", 5.0f } } },
                      { "reverb", false, { { "type", 2.0f }, { "tone", 6.0f }, { "decay", 4.5f } } } };
        m.fxControls = { ParamSpec::knob ("boost", "Boost", 3.0f), ParamSpec::knob ("delay", "Delay", 2.0f),
                         ParamSpec::knob ("reverb", "Reverb", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "gain", k[0] * 1.2f);
            fx.set (0, "bass", -k[0] * 0.8f);
            fx.set (1, "mix", k[1] * 4.5f);
            fx.set (1, "feedback", 18.0f + k[1] * 3.0f);
            fx.set (2, "mix", k[2] * 4.5f);
        };
        return m;
    }

    AmpModel synthwave()
    {
        AmpModel m;
        m.id = "synthwave"; m.titleAccent = "SYNTHWAVE"; m.titleRest = "AMP"; m.name = "Synthwave Amp";
        m.tagline = "NEON CLEAN  /  WIDE CHORUS  /  SYNCED ECHO  /  JET FLANGER";
        m.description = "Neon nights on the coast highway: a glassy 80s clean amp with a wide stereo chorus, a jet flanger, a tempo-synced dotted-eighth echo and a big plate.";
        m.look = { 0xff120a24, 0xff2a1450, 0xff160a2e, 0xff2ee6ff, false };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f;
        v.gainCompensation = 0.3; v.masterCompensation = 0.5; v.outputTrim = 0.63;
        v.inputHighpassHz = 50.0; v.brightHz = 3000.0; v.brightDb = 3.0;
        v.stages = { { 2.2, 0.08, 20.0, 18000.0 }, { 3.0, 0.15, 25.0, 13000.0 } };
        v.gainMinDb = -24.0f; v.gainMaxDb = 8.0f;
        v.toneStack = ToneStack::blackface();
        v.sag = 0.4; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 4.0f; m.defaultBass = 4.5f; m.defaultTreble = 6.5f;

        m.effects = { { "chorus", false, { { "mode", 2.0f }, { "rate", 0.5f }, { "tone", 8.0f } } },
                      { "flanger", false, { { "rate", 0.2f }, { "depth", 6.0f }, { "manual", 4.0f }, { "regen", 40.0f } } },
                      { "delay", false, { { "mode", 2.0f }, { "sync", 5.0f }, { "tone", 6.0f } } },
                      { "reverb", false, { { "type", 2.0f }, { "tone", 7.0f }, { "decay", 6.0f }, { "predelay", 20.0f } } } };
        m.fxControls = { ParamSpec::knob ("chorus", "Chorus", 5.0f), ParamSpec::knob ("jet", "Jet", 2.0f),
                         ParamSpec::knob ("echo", "Echo", 4.0f), ParamSpec::knob ("reverb", "Reverb", 4.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "mix", k[0] * 7.0f);
            fx.set (0, "depth", 3.0f + k[0] * 0.5f);
            fx.set (1, "mix", k[1] * 5.0f);
            fx.set (2, "mix", k[2] * 4.5f);
            fx.set (2, "feedback", 25.0f + k[2] * 3.0f);
            fx.set (3, "mix", k[3] * 5.5f);
        };
        return m;
    }

    AmpModel western()
    {
        AmpModel m;
        m.id = "western"; m.titleAccent = "WESTERN"; m.titleRest = "AMP"; m.name = "Western Amp";
        m.tagline = "TWANG  /  SLAPBACK  /  TREMOLO  /  SPRING";
        m.description = "Dust, sunsets and big low strings: a twangy clean amp with a slapback echo, amp tremolo and a spring reverb. Spaghetti-western themes, rockabilly and desert rock.";
        m.look = { 0xff2a160c, 0xffc98f55, 0xff9a6236, 0xff7a2e12, true };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f;
        v.gainCompensation = 0.3; v.masterCompensation = 0.5; v.outputTrim = 0.46;
        v.inputHighpassHz = 60.0; v.brightHz = 2500.0; v.brightDb = 5.0;
        v.stages = { { 2.4, 0.10, 25.0, 17000.0 }, { 3.2, 0.18, 30.0, 12000.0 } };
        v.gainMinDb = -24.0f; v.gainMaxDb = 10.0f;
        v.toneStack = ToneStack::blackface();
        v.sag = 0.6; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 4.5f; m.defaultMiddle = 4.0f; m.defaultTreble = 7.0f;

        m.effects = { { "delay", false, { { "mode", 1.0f }, { "time", 115.0f }, { "feedback", 8.0f }, { "tone", 5.0f } } },
                      { "tremolo", false, { { "shape", 3.0f }, { "mode", 0.0f } } },
                      { "reverb", false, { { "type", 3.0f }, { "tone", 6.0f }, { "decay", 4.0f } } } };
        m.fxControls = { ParamSpec::knob ("slapback", "Slapback", 4.0f), ParamSpec::knob ("tremolo", "Tremolo", 3.0f),
                         ParamSpec::range ("speed", "Speed", 1.0f, 12.0f, 4.5f, "Hz", 1, 4.0f),
                         ParamSpec::knob ("spring", "Spring", 4.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "mix", k[0] * 5.0f);
            fx.set (1, "depth", k[1] * 10.0f);
            fx.set (1, "rate", k[2]);
            fx.set (2, "mix", k[3] * 6.0f);
        };
        return m;
    }

    AmpModel funk()
    {
        AmpModel m;
        m.id = "funk"; m.titleAccent = "FUNK"; m.titleRest = "AMP"; m.name = "Funk Amp";
        m.tagline = "SQUEAKY CLEAN  /  COMPRESSOR  /  ENVELOPE FILTER  /  PHASER";
        m.description = "Disco-ball cleans: a bright, punchy clean amp with a squashy compressor and an envelope filter that quacks with your picking, plus a phaser. Funk, disco and neo-soul.";
        m.look = { 0xff1d0f24, 0xff6b2a7a, 0xff3e1650, 0xffffc23a, false };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f;
        v.gainCompensation = 0.3; v.masterCompensation = 0.5; v.outputTrim = 0.59;
        v.inputHighpassHz = 70.0; v.brightHz = 2800.0; v.brightDb = 4.0;
        v.stages = { { 2.0, 0.08, 25.0, 18000.0 }, { 2.8, 0.15, 30.0, 13000.0 } };
        v.gainMinDb = -24.0f; v.gainMaxDb = 8.0f;
        v.toneStack = ToneStack::blackface();
        v.sag = 0.3; v.defaultCabinet = CabinetSection::Combo1x12;
        m.defaultGain = 4.0f; m.defaultBass = 4.0f; m.defaultTreble = 7.0f;

        m.effects = { { "compressor", true, { { "attack", 2.0f }, { "tone", 6.0f }, { "blend", 80.0f } } },
                      { "envfilter", true, { { "resonance", 6.0f }, { "mode", 0.0f }, { "direction", 0.0f } } },
                      { "phaser", false, { { "stages", 0.0f }, { "depth", 6.0f }, { "feedback", 30.0f }, { "rate", 0.6f } } } };
        m.fxControls = { ParamSpec::knob ("squeeze", "Squeeze", 5.0f), ParamSpec::knob ("quack", "Quack", 5.0f),
                         ParamSpec::knob ("range", "Range", 3.0f), ParamSpec::knob ("phase", "Phase", 2.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "sustain", k[0]);
            fx.set (1, "mix", k[1] * 10.0f);
            fx.set (1, "sensitivity", 3.0f + k[1] * 0.5f);
            fx.set (1, "range", k[2]);
            fx.set (2, "mix", k[3] * 5.0f);
        };
        return m;
    }

    AmpModel pharaoh()
    {
        AmpModel m;
        m.id = "pharaoh"; m.titleAccent = "PHARAOH"; m.titleRest = "AMP"; m.name = "Pharaoh Amp";
        m.tagline = "DESERT CRUNCH  /  TAPE ECHO  /  MIRAGE  /  TEMPLE HALL";
        m.description = "A warm, nasal desert crunch with an oud-like bite in the mids, a two-head tape echo, a slow mirage phaser and the reverb of a stone temple.";
        m.look = { 0xff171007, 0xffd9b46a, 0xffa77e35, 0xff1f4fa8, true };

        auto& v = m.voicing;
        v.outputTrim = 0.32;
        v.inputHighpassHz = 60.0; v.brightHz = 2000.0; v.brightDb = 2.0;
        v.preMidHz = 1100.0; v.preMidQ = 1.2; v.preMidDb = 6.0;
        v.stages = { { 4.5, 0.15, 30.0, 14000.0 }, { 7.0, 0.30, 70.0, 9000.0 }, { 1.6, 0.20, 50.0, 8000.0 } };
        v.gainMinDb = -32.0f; v.gainMaxDb = 6.0f;
        v.toneStack = ToneStack::bassman();
        v.sag = 0.5; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 5.0f; m.defaultMiddle = 6.5f;

        m.effects = { { "tapeecho", false, { { "heads", 3.0f }, { "wobble", 3.0f }, { "age", 4.0f }, { "rate", 320.0f } } },
                      { "phaser", false, { { "stages", 2.0f }, { "depth", 6.0f }, { "feedback", 40.0f }, { "rate", 0.15f } } },
                      { "reverb", false, { { "type", 1.0f }, { "tone", 5.0f }, { "predelay", 35.0f } } } };
        m.fxControls = { ParamSpec::knob ("echo", "Echo", 4.0f), ParamSpec::knob ("mirage", "Mirage", 3.0f),
                         ParamSpec::knob ("temple", "Temple", 5.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "echo", k[0] * 8.0f);
            fx.set (0, "intensity", 3.0f + k[0] * 0.3f);
            fx.set (1, "mix", k[1] * 5.0f);
            fx.set (2, "mix", k[2] * 5.5f);
            fx.set (2, "decay", 4.0f + k[2] * 0.5f);
        };
        return m;
    }

    AmpModel sakura()
    {
        AmpModel m;
        m.id = "sakura"; m.titleAccent = "SAKURA"; m.titleRest = "AMP"; m.name = "Sakura Amp";
        m.tagline = "CITY-POP CLEAN  /  COMPRESSOR  /  CHORUS  /  ECHO  /  PLATE";
        m.description = "Cherry blossoms and late-night drives: a polished, glassy clean amp with a gentle compressor, a shimmering chorus, a soft echo and a plate reverb. City pop, J-rock cleans and anime openings.";
        m.look = { 0xff1a1216, 0xfff1d6dc, 0xffd9aeb9, 0xffc8102e, true };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f;
        v.gainCompensation = 0.3; v.masterCompensation = 0.5; v.outputTrim = 0.43;
        v.inputHighpassHz = 50.0; v.brightHz = 3000.0; v.brightDb = 3.0;
        v.stages = { { 2.2, 0.08, 20.0, 18000.0 }, { 3.0, 0.15, 25.0, 13000.0 } };
        v.gainMinDb = -24.0f; v.gainMaxDb = 8.0f;
        v.toneStack = ToneStack::blackface();
        v.sag = 0.35; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 4.5f; m.defaultBass = 4.5f; m.defaultMiddle = 5.5f; m.defaultTreble = 6.5f;

        m.effects = { { "compressor", true, { { "attack", 4.0f }, { "tone", 6.0f }, { "blend", 60.0f } } },
                      { "chorus", false, { { "mode", 0.0f }, { "rate", 0.9f }, { "tone", 7.0f } } },
                      { "delay", false, { { "mode", 0.0f }, { "time", 360.0f }, { "tone", 6.0f }, { "feedback", 25.0f } } },
                      { "reverb", false, { { "type", 2.0f }, { "tone", 6.0f }, { "decay", 4.0f } } } };
        m.fxControls = { ParamSpec::knob ("squeeze", "Squeeze", 4.0f), ParamSpec::knob ("chorus", "Chorus", 4.0f),
                         ParamSpec::knob ("echo", "Echo", 2.0f), ParamSpec::knob ("plate", "Plate", 3.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "sustain", k[0]);
            fx.set (1, "mix", k[1] * 7.0f);
            fx.set (1, "depth", 3.0f + k[1] * 0.4f);
            fx.set (2, "mix", k[2] * 4.0f);
            fx.set (3, "mix", k[3] * 5.0f);
        };
        return m;
    }

    AmpModel persian()
    {
        AmpModel m;
        m.id = "persian"; m.titleAccent = "PERSIAN"; m.titleRest = "AMP"; m.name = "Persian Amp";
        m.tagline = "SILKY CLEAN  /  OCTAVE SPARKLE  /  ECHO  /  PALACE HALL";
        m.description = "Turquoise tiles and golden light: a bright, silky clean amp with an octave-up sparkle reminiscent of a hammered santur, a soft echo and a palace hall.";
        m.look = { 0xff0b1a2a, 0xff12507a, 0xff0b3352, 0xffe6c068, false };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f;
        v.gainCompensation = 0.3; v.masterCompensation = 0.5; v.outputTrim = 0.43;
        v.inputHighpassHz = 50.0; v.brightHz = 2800.0; v.brightDb = 4.0;
        v.preMidHz = 900.0; v.preMidQ = 0.9; v.preMidDb = 2.0;
        v.stages = { { 2.4, 0.10, 25.0, 17000.0 }, { 3.4, 0.18, 30.0, 12000.0 } };
        v.gainMinDb = -24.0f; v.gainMaxDb = 8.0f;
        v.toneStack = ToneStack::blackface();
        v.sag = 0.45; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 4.5f; m.defaultTreble = 6.5f;

        m.effects = { { "pitch", false, { { "interval", 12.0f }, { "fine", 4.0f }, { "bend", 100.0f }, { "tone", 8.0f } } },
                      { "delay", false, { { "mode", 1.0f }, { "time", 330.0f }, { "tone", 6.0f }, { "feedback", 30.0f } } },
                      { "reverb", false, { { "type", 1.0f }, { "tone", 6.0f }, { "predelay", 20.0f } } } };
        m.fxControls = { ParamSpec::knob ("sparkle", "Sparkle", 4.0f), ParamSpec::knob ("echo", "Echo", 3.0f),
                         ParamSpec::knob ("hall", "Hall", 4.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "mix", k[0] * 4.5f);
            fx.set (1, "mix", k[1] * 4.5f);
            fx.set (2, "mix", k[2] * 5.5f);
            fx.set (2, "decay", 3.5f + k[2] * 0.5f);
        };
        return m;
    }

    AmpModel choir()
    {
        AmpModel m;
        m.id = "choir"; m.titleAccent = "CHOIR"; m.titleRest = "AMP"; m.name = "Choir Amp";
        m.tagline = "HEAVENLY CLEAN  /  CHOIR SHIMMER  /  CATHEDRAL";
        m.description = "Light through stained glass: a soft, warm clean amp whose reverb rises into a choir of octaves and fifths, inside an endless cathedral. Worship, ambient and post-rock swells.";
        m.look = { 0xff151320, 0xffe9e4d6, 0xffc9c1ad, 0xff3d5bd9, true };

        auto& v = m.voicing;
        v.masterMaxDb = 6.0f;
        v.gainCompensation = 0.4; v.masterCompensation = 0.5; v.outputTrim = 0.65;
        v.inputHighpassHz = 40.0; v.brightHz = 2000.0; v.brightDb = 1.0;
        v.stages = { { 2.0, 0.10, 20.0, 16000.0 }, { 2.6, 0.15, 25.0, 11000.0 } };
        v.gainMinDb = -24.0f; v.gainMaxDb = 8.0f;
        v.toneStack = ToneStack::blackface();
        v.sag = 0.5; v.defaultCabinet = CabinetSection::Open2x12;
        m.defaultGain = 4.0f; m.defaultTreble = 5.5f;

        m.effects = { { "shimmer", false, { { "interval", 2.0f }, { "tone", 5.0f } } },
                      { "reverb", false, { { "type", 1.0f }, { "tone", 5.0f }, { "predelay", 40.0f } } } };
        m.fxControls = { ParamSpec::knob ("choir", "Choir", 6.0f),
                         ParamSpec::choice ("voices", "Voices", { "Octave", "Fifth", "Oct+5th", "2 Oct" }, 2),
                         ParamSpec::knob ("cathedral", "Cathedral", 6.0f) };
        m.applyControls = [] (const float* k, const EffectSetter& fx)
        {
            fx.set (0, "shimmer", k[0] * 9.0f);
            fx.set (0, "mix", 15.0f + k[0] * 4.0f);
            fx.set (0, "decay", 5.0f + k[2] * 0.3f);
            fx.set (0, "interval", k[1]);
            fx.set (1, "mix", k[2] * 5.0f);
            fx.set (1, "decay", 5.0f + k[2] * 0.5f);
        };
        return m;
    }
}

const std::vector<AmpModel>& allAmpModels()
{
    // Order is the display order. Ids are stable; append new amps at the end.
    static const std::vector<AmpModel> models { dream(), dub(), spaceDelay(), blackMetal(), warmRock(),
                                                surf(), shoegaze(), arena(), stereoJazz(), doom(),
                                                crunch(), eightBit(), knight(), heavyMetal(), synthwave(),
                                                western(), funk(), pharaoh(), sakura(), persian(), choir() };
    return models;
}

const AmpModel* findAmpModel (const std::string& id)
{
    for (const auto& m : allAmpModels())
        if (m.id == id)
            return &m;
    return nullptr;
}

} // namespace oap::amps
