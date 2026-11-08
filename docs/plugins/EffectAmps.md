# OAP Effect Amps

<p align="center"><img src="../assets/effect-amps.gif" width="100%" alt="The ten OAP effect amps, one after another"></p>

Ten complete "amp + effects" plugins, each built around one sound. Every
effect amp is a tube-style amp head with its own voicing, plus the effects that
genre is known for, already dialled in and reduced to a few knobs. Load one,
play, done.

| Plugin | Sound | Built-in effects | Gate |
| --- | --- | --- | --- |
| [Dream Amp](#dream-amp) | Glassy clean, dream pop, ambient | stereo chorus, ping-pong echo, shimmer reverb | |
| [Dub Amp](#dub-amp) | Reggae skank, dub | tape echo, spring | |
| [Space Delay Amp](#space-delay-amp) | Warm crunch, space rock | three-head tape echo, spring | |
| [Black Metal Amp](#black-metal-amp) | Cold buzzsaw high gain | hall reverb | ✓ |
| [Warm Rock Amp](#warm-rock-amp) | British crunch, classic rock | analog delay, plate | |
| [Surf Amp](#surf-amp) | Bright twangy clean | spring tank, tremolo | |
| [Shoegaze Amp](#shoegaze-amp) | Fuzz wall, noise pop | fuzz, vibrato, huge hall | |
| [Arena Amp](#arena-amp) | 80s hot-rod lead | stereo chorus, ping-pong delay, plate | ✓ |
| [Stereo Jazz Amp](#stereo-jazz-amp) | Solid-state clean, jazz, funk | stereo chorus/vibrato, spring | |
| [Doom Amp](#doom-amp) | Fuzz into a cranked amp, doom, stoner | fuzz, slow phaser, room | ✓ |

## How every effect amp works

```text
input ─ [Gate] ─ [fuzz, if built in] ─ voicing EQ ─ tube preamp ─ tone stack ─ power amp ─ cabinet ─ stereo effects ─ Output
                                                    └──────── 4x or 8x oversampled ────────┘
```

- **The amp section is mono**, like a real amp. The effects after the cabinet
  run in **stereo**, so chorus, ping-pong delays and reverbs spread across
  both speakers on a stereo track.
- **Fuzz** sits in front of the amp (Shoegaze, Doom), like a pedal.
- The **effects come after the cabinet**, like a studio effects rack, so
  delays and reverbs stay clean and wide instead of being distorted by the amp.

### Controls on every effect amp

| Control | Range | What it does |
| --- | --- | --- |
| Gate *(some amps)* | Off, -89 ... -30 dB | Noise gate threshold. Fully left switches it off. |
| Gain | 0-10 | How hard the preamp is driven. |
| Bass / Middle / Treble | 0-10 | Passive amp tone stack: the knobs interact like on a real amp. |
| Master | 0-10 | Power amp drive: more compression, sag and bloom as it goes up. |
| *Effect knobs* | | Two to five per amp, see below. |
| Output | -24 ... +12 dB | Final level. |
| Cabinet | Off, 4x12 Closed, 2x12 Open, 1x12 Combo, User IR | Speaker cabinet. **Load IR…** loads your own impulse response. |

Double-click a knob to reset it, hold **Shift** while dragging for fine
control, and drag the window's corner to resize it. The amps are level-matched:
clean amps sit around -19 dB RMS and driven ones around -15 dB RMS with a
typical guitar DI, so switching amps doesn't blast you.

---

## Dream Amp

<p align="center"><img src="../assets/amps/dream.png" width="100%" alt="OAP Dream Amp"></p>

A glassy clean amp floating in a wide chorus, ping-pong echoes and an
octave-up shimmer reverb. Instant dream pop and ambient swells.

| Effect knob | What it does |
| --- | --- |
| Chorus | Wide stereo chorus: amount and depth. |
| Echo | Ping-pong echo level and number of repeats. |
| Shimmer | How much of the reverb tail rises an octave. |
| Space | Reverb size and level: from a room to endless. |

**Try:** Gain 3, Treble 7, Chorus 5, Echo 5, Shimmer 7, Space 9. Play slow chords and let them ring.

## Dub Amp

<p align="center"><img src="../assets/amps/dub.png" width="100%" alt="OAP Dub Amp"></p>

A tight, bright clean amp for reggae skanks (the low end is trimmed before the
preamp so offbeat chops stay crisp) with a dub tape echo you can push into
endless repeats, and a short spring reverb.

| Effect knob | What it does |
| --- | --- |
| Echo Time | 120-700 ms. |
| Feedback | Number of repeats. At the very top the echo runs away into self-oscillation: automate it for dub throws. |
| Echo | Echo level. |
| Spring | Spring reverb amount. |

**Try:** Bass 2, Middle 6, Treble 7, Echo Time 375 ms (a dotted eighth at 120 BPM), Feedback 6, Echo 5.

## Space Delay Amp

<p align="center"><img src="../assets/amps/spacedelay.png" width="100%" alt="OAP Space Delay Amp"></p>

A warm crunch amp with a three-head tape echo and a spring reverb built in,
from slapback to self-oscillating space trips.

| Effect knob | What it does |
| --- | --- |
| Repeat Rate | Tape speed: 50-700 ms between repeats (all heads move together). |
| Intensity | Repeats. High settings self-oscillate. |
| Echo | Echo level. |
| Heads | Which playback heads you hear: 1, 2, 3, 1+2, 2+3 or 1+2+3, for different rhythmic patterns. |
| Reverb | Spring reverb amount. |

**Try:** Gain 4, Repeat Rate 300 ms, Heads 1+2+3, Intensity 6 and slowly turn it up while you hold a chord.

## Black Metal Amp

<p align="center"><img src="../assets/amps/blackmetal.png" width="100%" alt="OAP Black Metal Amp"></p>

Thin, cold and razor sharp: a trebly buzzsaw high-gain amp with scooped mids,
a fast gate and a dark cathedral hall for tremolo-picked riffs.
Oversampled 8x so the extreme gain stays smooth.

| Effect knob | What it does |
| --- | --- |
| Hall | Hall reverb amount. |
| Decay | Hall length, up to cathedral. |

**Try:** Gate -55 dB, Gain 8, Bass 4, Middle 3, Treble 7, Hall 5, Decay 8.

## Warm Rock Amp

<p align="center"><img src="../assets/amps/warmrock.png" width="100%" alt="OAP Warm Rock Amp"></p>

A warm British-style crunch amp, from rhythm crunch to singing classic-rock
leads, with an analog-style delay and a studio plate reverb.

| Effect knob | What it does |
| --- | --- |
| Delay | Analog delay level and repeats. |
| Delay Time | 80-900 ms. |
| Plate | Plate reverb amount. |

**Try:** rhythm with Gain 4, Delay 0, Plate 2; lead with Gain 8, Middle 7, Delay 4 at 450 ms, Plate 3.

## Surf Amp

<p align="center"><img src="../assets/amps/surf.png" width="100%" alt="OAP Surf Amp"></p>

Sparkling clean tone drenched in a drippy spring reverb tank, with an
amp-style tremolo. Made for twangy instrumentals.

| Effect knob | What it does |
| --- | --- |
| Dwell | How hard the spring tank is driven: longer, drippier tails. |
| Reverb | Spring reverb level. |
| Speed | Tremolo speed, 1-12 Hz. |
| Intensity | Tremolo depth (0 = off). |

**Try:** Bass 3, Treble 8, Dwell 8, Reverb 7, Intensity 0. Pick hard on the low strings near the bridge.

## Shoegaze Amp

<p align="center"><img src="../assets/amps/shoegaze.png" width="100%" alt="OAP Shoegaze Amp"></p>

A wall of fuzz into a cranked amp, warbling vibrato and a huge washed-out
hall: shoegaze and noise pop in one box.

| Effect knob | What it does |
| --- | --- |
| Fuzz | Fuzz pedal in front of the amp (level-compensated). |
| Warble | Vibrato depth: from gentle detune to seasick pitch bends. |
| Wash | Hall reverb amount. |
| Size | Hall length. |

**Try:** Fuzz 7, Warble 5, Wash 8, Size 9, Treble 5. Strum big open chords and bend the neck.

## Arena Amp

<p align="center"><img src="../assets/amps/arena.png" width="100%" alt="OAP Arena Amp"></p>

The big 80s rack sound: a hot-rodded lead amp into a stereo chorus, a
ping-pong delay and a plate reverb. Oversampled 8x, with a gate.

| Effect knob | What it does |
| --- | --- |
| Chorus | Stereo chorus amount. |
| Delay | Ping-pong delay level and repeats (about 430 ms). |
| Reverb | Plate reverb amount. |

**Try:** Gain 7, Middle 6, Treble 6.5, Chorus 4, Delay 5, Reverb 4. Play a solo with lots of vibrato.

## Stereo Jazz Amp

<p align="center"><img src="../assets/amps/stereojazz.png" width="100%" alt="OAP Stereo Jazz Amp"></p>

A pristine solid-state clean amp with a lush built-in stereo chorus and
vibrato. Jazz chords, funk and new-wave cleans. It stays clean almost all the
way up.

| Effect knob | What it does |
| --- | --- |
| Speed | Chorus/vibrato speed, 0.1-6 Hz. |
| Depth | Modulation depth (0 = dry). |
| Vibrato | Off: stereo chorus. On: pitch vibrato. |
| Reverb | Spring reverb amount. |

**Try:** Gain 4, Bass 6, Treble 5, Speed 0.6 Hz, Depth 6 for chorus; Vibrato on with Speed 5 Hz and Depth 4 for a warbly vibrato.

## Doom Amp

<p align="center"><img src="../assets/amps/doom.png" width="100%" alt="OAP Doom Amp"></p>

Low and slow: a thick fuzz into a cranked, saggy amp with a huge low end, a
slow phaser and a dark room. Stoner, doom and sludge.

| Effect knob | What it does |
| --- | --- |
| Fuzz | Fuzz pedal in front of the amp (level-compensated). |
| Phase | Phaser amount (0 = off). |
| Phase Speed | 0.05-2 Hz. |
| Room | Room reverb amount. |

**Try:** Gate -60 dB, Fuzz 8, Gain 6, Bass 7, Treble 4, Phase 4 at 0.15 Hz. Tune down and play slowly.

---

## Technical

- Mono amp, stereo effects after the cabinet (mono in, stereo out works best).
- 4x oversampling (8x on Black Metal and Arena), polyphase IIR.
- Latency: 6 samples, or 45 samples on the amps with a built-in fuzz
  (Shoegaze, Doom). Always reported to the host for compensation.
- CPU: about 2.5-7 % of one core at 48 kHz, measured on an x86 cloud machine.
- Tails of up to 20 s (reverbs and echoes), reported to the host.
- Every amp is described by one entry in
  [`shared/amps/AmpModels.cpp`](../../shared/amps/AmpModels.cpp); see the
  [developer guide](../DEVELOPMENT.md#adding-an-effect-amp) to make your own.
