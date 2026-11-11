<p align="center">
  <img src="docs/assets/hero.svg" width="100%" alt="Open Audio Plugins: free guitar amps and effects for your DAW">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/status-early%20development-ff5a1f?style=flat-square" alt="Status: early development">
  <img src="https://img.shields.io/badge/formats-VST3%20%7C%20AU%20%7C%20Standalone-19b3a3?style=flat-square" alt="Formats: VST3, AU, Standalone">
  <img src="https://img.shields.io/badge/macOS%20%7C%20Windows%20%7C%20Linux-555?style=flat-square" alt="Platforms: macOS, Windows, Linux">
  <img src="https://img.shields.io/badge/built%20with-JUCE%209%20%C2%B7%20C%2B%2B17-555?style=flat-square" alt="Built with JUCE 9 and C++17">
  <img src="https://img.shields.io/badge/plugins-45-ff5a1f?style=flat-square" alt="45 plugins">
  <img src="https://img.shields.io/badge/price-free-2ea44f?style=flat-square" alt="Price: free">
</p>

<p align="center">
  <b>Plug in your guitar, open your DAW, turn some knobs.</b><br>
  Open Audio Plugins is a growing collection of free, open audio plugins: practical tools with clear interfaces,<br>
  reliable performance and no unnecessary restrictions. 🎸<br><br>
  <b>2 amps · 21 effect amps · 20 pedals · 1 Pedalboard · 1 Looper</b>, every amp with its own artwork<br>
  VST3 · Audio Unit · Standalone, on macOS, Windows and Linux
</p>

<p align="center">
  <a href="#the-collection">The collection</a> ·
  <a href="#works-with">Works with</a> ·
  <a href="#quick-start">Quick start</a> ·
  <a href="#step-by-step">Step-by-step guide</a> ·
  <a href="#tone-recipes">Tone recipes</a> ·
  <a href="#troubleshooting">Troubleshooting</a> ·
  <a href="#roadmap">Roadmap</a>
</p>

> [!NOTE]
> **Early development.** There are no downloadable builds yet, so for now you build the plugins yourself.
> Don't worry: it's copy-paste, about 15 minutes, mostly waiting. The [step-by-step guide](#step-by-step) covers every click.

---

<a id="the-collection"></a>
## 🎛️ The collection

| | What | Plugins |
| --- | --- | --- |
| 🔥💎 | **[Amps](#amps)** | Drive Amp, Clean Amp |
| 🌈 | **[Effect amps](#effect-amps)**: an amp and its genre's effects in one plugin | Dream, Dub, Space Delay, Black Metal, Warm Rock, Surf, Shoegaze, Arena, Stereo Jazz, Doom, Crunch, 8-Bit, Knight, Heavy Metal, Synthwave, Western, Funk, Pharaoh, Sakura, Persian, Choir |
| 🦶 | **[Pedals](#pedals)**: 12 everyday + 8 advanced stompboxes | Compressor, Boost, Overdrive, Distortion, Fuzz, Chorus, Flanger, Phaser, Tremolo, Delay, Reverb, Wah · Shimmer, Tape Echo, Pitch Shifter, Envelope Filter, Rotary, Freeze, Guitar Synth, Lo-Fi |
| 🧩 | **[Pedalboard](#pedalboard)**: any eight pedals in one plugin | Pedalboard |
| 🔁 | **[Looper](#looper)**: record a phrase, play over it, layer after layer | Looper |

Every plugin is separate, so you can mix them freely on a track: pedals into an amp, an effect amp on its own, the Pedalboard into the Drive Amp, the Looper after any of them, or OAP pedals in front of any other amp sim.

<a id="amps"></a>
### 🔥 OAP Drive Amp: the high-gain monster

<p align="center">
  <img src="docs/assets/drive-amp-demo.gif" width="100%" alt="OAP Drive Amp: knobs being turned, the boost switched on, and every knob double-clicked back to its default">
</p>

From chunky crunch to fully saturated, sustaining leads.

| Feature | What it does |
| --- | --- |
| 🔇 **Gate** | Kills hiss between riffs, opens instantly on your pick attack. |
| 🎯 **Tight + Boost** | Trims the low end before the distortion and adds an overdrive-pedal push, for chugs that stay tight. |
| 🔥 **4-stage tube preamp** | Four cascaded tube-style stages, 8x oversampled so the top end stays smooth, not fizzy. |
| 🎚️ **Bass / Middle / Treble** | A model of a real passive amp tone stack, so the knobs interact like on a real amp. |
| 💥 **Presence · Resonance · Master** | Power-amp bite, low-end thump, and power-tube saturation with sag. |
| 🔈 **Cabinet** | Three built-in cabs (4x12, 2x12, 1x12), or load your own impulse response. |

### 💎 OAP Clean Amp: sparkle, glass and bloom

<p align="center">
  <img src="docs/assets/clean-amp-demo.gif" width="100%" alt="OAP Clean Amp: Bright switched on, Volume, Treble, Reverb and Master turned up, then reset">
</p>

Pristine cleans that get warm and hairy when you turn the Volume up and dig in.

| Feature | What it does |
| --- | --- |
| ✨ **Bright** | Extra sparkle at low volume. It fades out as you turn up, just like the real circuit. |
| 🔊 **Volume** | Clean up to about 6, then edge-of-break-up territory. |
| 🎚️ **Bass / Middle / Treble** | Blackface-style tone stack: big lows, glassy highs. |
| 🌊 **Reverb** | Stereo room reverb that stays out of the mud. |
| 🫧 **Master** | Power amp with a soft, spongy sag. |
| 🔈 **Cabinet** | 2x12 open-back by default, or bring your own impulse response. |

<a id="effect-amps"></a>
### 🌈 Effect amps: one sound, one plugin

<p align="center">
  <img src="docs/assets/effect-amps.gif" width="100%" alt="Every OAP effect amp shown one after another">
</p>

Each effect amp is a complete rig built around one sound: an amp voiced for the genre, plus the effects it's known for, already dialled in and boiled down to a few knobs. And each one has its own artwork, from a space dream to a horned skull, so you can tell them apart at a glance. Load one and play.

| Amp | Sounds like | Built-in effects | Artwork |
| --- | --- | --- | --- |
| 🌌 **Dream Amp** | Glassy dream-pop and ambient cleans | wide chorus, ping-pong echo, shimmer reverb | moon, planets and dream clouds |
| 🌴 **Dub Amp** | Crisp reggae skanks and dub throws | tape echo that runs away, spring | sunburst, palms and a sound system |
| 🛸 **Space Delay Amp** | Warm crunch for space rock | three-head tape echo, spring | rocket, ringed planet and tape reels |
| 🖤 **Black Metal Amp** | Cold, trebly buzzsaw | gate, cathedral hall | horned skull, pentagram and a dead forest |
| 🥃 **Warm Rock Amp** | British crunch and classic-rock leads | analog delay, plate | winged pick, sunset, vinyl and flames |
| 🏄 **Surf Amp** | Twangy, drippy instrumentals | spring tank, tremolo | a curling wave, palms and surfboards |
| 🌫️ **Shoegaze Amp** | A wall of fuzz and wash | fuzz, vibrato, huge hall | hazy bokeh and blurred flowers |
| 🏟️ **Arena Amp** | 80s stadium leads | gate, stereo chorus, ping-pong delay, plate | spotlights and a crowd |
| 🎷 **Stereo Jazz Amp** | Pristine solid-state cleans | stereo chorus / vibrato, spring | a night skyline and floating notes |
| 🌋 **Doom Amp** | Fuzz into a cranked amp, low and slow | gate, fuzz, slow phaser, room | volcanoes and a black eclipse |
| 🍸 **Crunch Amp** | Plexi-style crunch, rock'n'roll, punk | treble booster, room | an art-deco cocktail bar |
| 👾 **8-Bit Amp** | Chiptune and game music | square-wave synth, bit crusher, echo | pixel hearts, coins and bricks |
| 🛡️ **Knight Amp** | Heroic power-metal leads | gate, harmony voice, ping-pong echo, hall | a castle and a dragon across the moon |
| ⚡ **Heavy Metal Amp** | 80s heavy metal | gate, tight boost, delay, plate | a lightning storm |
| 🌆 **Synthwave Amp** | Neon 80s cleans | wide chorus, flanger, tempo-synced echo, plate | a neon sun over a grid |
| 🤠 **Western Amp** | Twang and desert themes | slapback, tremolo, spring | mesas, cacti and a sheriff's star |
| 🪩 **Funk Amp** | Funk, disco and neo-soul | compressor, envelope filter, phaser | a disco ball and 70s stripes |
| 🏺 **Pharaoh Amp** | Desert crunch with an oud-like bite | tape echo, slow phaser, temple hall | pyramids and a sun with an eye |
| 🌸 **Sakura Amp** | City pop and J-rock cleans | compressor, chorus, echo, plate | Mt Fuji, cherry blossoms and waves |
| 🕌 **Persian Amp** | Silky clean with an octave sparkle | octave-up voice, echo, palace hall | tile stars and a golden arch |
| ⛪ **Choir Amp** | Heavenly swells | choir shimmer, cathedral reverb | a rose window, arches and candles |

Every effect amp also has **Gain, Bass, Middle, Treble, Master, Output** and the **cabinet / IR loader**. 📖 [Effect amp manual](docs/plugins/EffectAmps.md)

<details>
<summary><b>🖼️ See every amp</b></summary>
<br>

<table>
  <tr>
    <td width="50%"><img src="docs/assets/amps/dream.png" width="100%" alt="OAP Dream Amp"><br><sub>🌌 <b>Dream Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/dub.png" width="100%" alt="OAP Dub Amp"><br><sub>🌴 <b>Dub Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/spacedelay.png" width="100%" alt="OAP Space Delay Amp"><br><sub>🛸 <b>Space Delay Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/blackmetal.png" width="100%" alt="OAP Black Metal Amp"><br><sub>🖤 <b>Black Metal Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/warmrock.png" width="100%" alt="OAP Warm Rock Amp"><br><sub>🥃 <b>Warm Rock Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/surf.png" width="100%" alt="OAP Surf Amp"><br><sub>🏄 <b>Surf Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/shoegaze.png" width="100%" alt="OAP Shoegaze Amp"><br><sub>🌫️ <b>Shoegaze Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/arena.png" width="100%" alt="OAP Arena Amp"><br><sub>🏟️ <b>Arena Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/stereojazz.png" width="100%" alt="OAP Stereo Jazz Amp"><br><sub>🎷 <b>Stereo Jazz Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/doom.png" width="100%" alt="OAP Doom Amp"><br><sub>🌋 <b>Doom Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/crunch.png" width="100%" alt="OAP Crunch Amp"><br><sub>🍸 <b>Crunch Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/eightbit.png" width="100%" alt="OAP 8-Bit Amp"><br><sub>👾 <b>8-Bit Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/knight.png" width="100%" alt="OAP Knight Amp"><br><sub>🛡️ <b>Knight Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/heavymetal.png" width="100%" alt="OAP Heavy Metal Amp"><br><sub>⚡ <b>Heavy Metal Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/synthwave.png" width="100%" alt="OAP Synthwave Amp"><br><sub>🌆 <b>Synthwave Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/western.png" width="100%" alt="OAP Western Amp"><br><sub>🤠 <b>Western Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/funk.png" width="100%" alt="OAP Funk Amp"><br><sub>🪩 <b>Funk Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/pharaoh.png" width="100%" alt="OAP Pharaoh Amp"><br><sub>🏺 <b>Pharaoh Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/sakura.png" width="100%" alt="OAP Sakura Amp"><br><sub>🌸 <b>Sakura Amp</b></sub></td>
    <td width="50%"><img src="docs/assets/amps/persian.png" width="100%" alt="OAP Persian Amp"><br><sub>🕌 <b>Persian Amp</b></sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/assets/amps/choir.png" width="100%" alt="OAP Choir Amp"><br><sub>⛪ <b>Choir Amp</b></sub></td>
    <td width="50%"></td>
  </tr>
</table>

</details>

<a id="pedals"></a>
### 🦶 Pedals: 12 everyday, 8 advanced

<p align="center">
  <img src="docs/assets/pedals-everyday.png" width="100%" alt="The twelve everyday OAP pedals">
</p>

**Everyday:** Compressor · Boost · Overdrive · Distortion · Fuzz · Chorus · Flanger · Phaser · Tremolo · Delay (tempo sync, analog, ping-pong) · Reverb (room, hall, plate, spring) · Wah (manual, auto, envelope)

<p align="center">
  <img src="docs/assets/pedals-advanced.png" width="70%" alt="The eight advanced OAP pedals">
</p>

**Advanced:**

| Pedal | What it does |
| --- | --- |
| ✨ **Shimmer** | Reverb whose tail climbs into octaves: instant cathedral pads. |
| 📼 **Tape Echo** | Three heads, wow and flutter, repeats that run away into self-oscillation. |
| 🎚️ **Pitch Shifter** | Octaves, harmonies and whammy-style bends. |
| 🦆 **Envelope Filter** | Funky auto-wah that follows your picking. |
| 🌀 **Rotary** | Rotating speaker that spins up and down. |
| 🧊 **Freeze** | Holds what you just played as an endless pad. |
| 🎹 **Guitar Synth** | Single notes become a fat analog synth with a sub octave. |
| 📻 **Lo-Fi** | Bit crushing, sample-rate reduction and tape wobble. |

Every pedal has a click-free footswitch that your DAW's bypass button controls too. 📖 [Pedal manual](docs/plugins/Pedals.md)

<a id="pedalboard"></a>
### 🧩 Pedalboard: eight slots, every pedal

<p align="center">
  <img src="docs/assets/pedalboard.png" width="100%" alt="OAP Pedalboard with Compressor, Overdrive, Chorus, Delay and Reverb loaded">
</p>

Pick any pedal for each of eight slots, reorder them with **<** and **>**, switch them on and off, and save the whole board with your project. Also a standalone app for practising. 📖 [Pedalboard manual](docs/plugins/Pedalboard.md)

<a id="looper"></a>
### 🔁 Looper: record a phrase, play over it

<p align="center">
  <img src="docs/assets/looper-overdub.png" width="100%" alt="OAP Looper overdubbing a second layer onto a two-bar loop">
</p>

Click the big button to record, click again and the phrase repeats, click again to play on top of it. Made for the mouse, with everything a looper needs for real playing:

- **Overdub, undo and redo**, double-click to stop, hold to undo: the big button works like a looper's footswitch.
- **In time with your project:** start and stop on the bar line while your DAW plays, record exact lengths (*2 beats*, *4 bars*…) that close by themselves, follow tempo changes, auto-start on your first note.
- **Auto loop, hands free:** one click, a count-in, and the Looper records a fixed length and then up to three more layers one loop cycle each, by itself.
- **A guide rhythm:** clicks (also 8ths, triplets, 16ths, shuffle, in any time signature) or simple drum grooves (rock, pop, funk, ballad, disco, shuffle, reggae, metal, bossa nova), never recorded into the loop.
- **Your loop is saved with the project**, and as a WAV file whenever you like. Drop an audio file on the window to loop it.
- **Half speed, double speed, reverse**, overdub feedback for loops that evolve, three ways to stop, and ×2 to make room for a longer layer.
- Switch parameters for **foot controllers**. Also a standalone app for practising.

📖 [Looper manual](docs/plugins/Looper.md)

<details>
<summary><b>Which plugin should I pick?</b></summary>
<br>

| You want… | Pick | Start with |
| --- | --- | --- |
| Metal, djent, hard rock | 🔥 Drive Amp | the defaults, then **Boost** on |
| Classic rock crunch | 🥃 Warm Rock Amp, or 🔥 Drive Amp | the defaults / Drive Amp **Gain** 2-3 |
| Funk, pop, indie cleans | 💎 Clean Amp | the defaults |
| Jazz, new-wave cleans with chorus | 🎷 Stereo Jazz Amp | the defaults |
| Blues, "just breaking up" | 💎 Clean Amp | **Volume** 7-8 |
| Ambient swells, post-rock | 🌌 Dream Amp | **Shimmer** and **Space** up |
| Reggae and dub | 🌴 Dub Amp | automate **Feedback** for dub throws |
| Black metal | 🖤 Black Metal Amp | the defaults |
| Doom, stoner, sludge | 🌋 Doom Amp | **Fuzz** 8, tune low |
| 80s heavy metal, thrash | ⚡ Heavy Metal Amp | **Boost** 5, **Middle** 7 |
| Power metal, epic solos | 🛡️ Knight Amp | **Harmony** 5 on a 5th |
| Punk, rock'n'roll | 🍸 Crunch Amp | **Gain** 7, **Booster** 5 |
| Chiptune, game music | 👾 8-Bit Amp | **Chip** 8, single notes |
| Funk, disco | 🪩 Funk Amp | **Quack** 7, muted strums |
| City pop, J-rock cleans | 🌸 Sakura Amp | the defaults |
| Synthwave, retro 80s | 🌆 Synthwave Amp | **Echo** 5, arpeggios |
| Western, rockabilly, twang | 🤠 Western Amp | low strings, **Slapback** 5 |
| Middle-Eastern melodies | 🏺 Pharaoh Amp / 🕌 Persian Amp | single-string slides / tremolo picking |
| Worship, ambient swells | ⛪ Choir Amp | **Choir** 8, volume swells |
| Your own rig | 🧩 Pedalboard → any amp | the default board |
| Practising solos over your own chords, building songs in layers | 🔁 Looper, after the amp | click the big button, play, click again; or **Auto Loop** *3 Layers* with the *Rock* pattern |

</details>

---

<a id="works-with"></a>
## 🖥️ Works with your DAW

| | 🍎 macOS 12+ | 🪟 Windows 10/11 | 🐧 Linux |
| --- | --- | --- | --- |
| **VST3** | ✓ | ✓ | ✓ |
| **Audio Unit** | ✓ | | |
| **Standalone** (amps, Pedalboard, Looper) | ✓ | ✓ | ✓ |

Any DAW that loads VST3 or Audio Units should work, including:

| DAW | macOS | Windows | Linux | Use |
| --- | :---: | :---: | :---: | --- |
| Ableton Live 11 / 12 (incl. Lite) | ✓ | ✓ | | VST3 or AU |
| Logic Pro · GarageBand · MainStage | ✓ | | | AU |
| REAPER | ✓ | ✓ | ✓ | VST3 |
| FL Studio | ✓ | ✓ | | VST3 |
| Cubase · Nuendo | ✓ | ✓ | | VST3 |
| Studio One | ✓ | ✓ | | VST3 or AU |
| Bitwig Studio | ✓ | ✓ | ✓ | VST3 |
| Cakewalk / Sonar | | ✓ | | VST3 |
| Ardour · Mixbus | ✓ | ✓ | ✓ | VST3 |
| Audacity 3.2+ | ✓ | ✓ | ✓ | VST3 |

Not yet: **Pro Tools** (needs AAX) and **iPad/iPhone** (needs AUv3).
Every plugin passes [pluginval](https://github.com/Tracktion/pluginval) at its strictest level; hands-on testing in each DAW is ongoing.
📖 **[Using the plugins in your DAW](docs/DAWS.md)**: rescan steps for each DAW and how to set up a guitar track.

---

<a id="quick-start"></a>
## ⚡ Quick start (if you've done this before)

<p align="center">
  <img src="docs/assets/terminal.svg" width="100%" alt="Terminal animation: installing CMake and Ninja, cloning the repository, configuring and building the plugins">
</p>

**🍎 macOS** (the plugins are copied into your plug-in folders automatically):

```bash
brew install cmake ninja
git clone https://github.com/mitradeniz/FreeAudioFX.git
cd FreeAudioFX
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

<details>
<summary><b>🪟 Windows</b> (Visual Studio 2022 with "Desktop development with C++", CMake, Git)</summary>
<br>

In a *Developer PowerShell for VS 2022*:

```powershell
git clone https://github.com/mitradeniz/FreeAudioFX.git
cd FreeAudioFX
cmake -B build
cmake --build build --config Release
```

Then, in a PowerShell window opened with **Run as administrator** (from the same folder), copy the plugins into the VST3 folder:

```powershell
Get-ChildItem build\plugins\*\*_artefacts\Release\VST3\*.vst3 |
    Copy-Item -Destination "C:\Program Files\Common Files\VST3" -Recurse -Force
```

</details>

<details>
<summary><b>🐧 Linux</b> (Debian / Ubuntu shown)</summary>
<br>

```bash
sudo apt install build-essential cmake ninja-build pkg-config git \
    libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
    libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev
git clone https://github.com/mitradeniz/FreeAudioFX.git
cd FreeAudioFX
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
mkdir -p ~/.vst3 && cp -r build/plugins/*/*_artefacts/Release/VST3/*.vst3 ~/.vst3/
```

</details>

Then **rescan plug-ins in your DAW** ([how, for each DAW](docs/DAWS.md)) and search for **OAP**. Rock. 🤘

Never used Terminal? No problem, read on. 👇

---

<a id="step-by-step"></a>
## 🪜 Step-by-step guide (macOS)

<p align="center">
  <img src="docs/assets/signal-flow.svg" width="100%" alt="Guitar into audio interface, into a DAW audio track, through the OAP plugins, out to headphones">
</p>

This guide walks through a Mac in detail. On Windows or Linux, follow the [quick start](#quick-start) above or the [build guide](docs/BUILDING.md), then continue at [step 7](#step-7).

### ✅ Step 0: Check what you need

| | You need | Notes |
| --- | --- | --- |
| 💻 | A Mac with **macOS 12** or newer | Apple Silicon or Intel |
| 🎹 | A **DAW** that loads VST3 or AU plug-ins | Ableton Live (Lite works too), Logic Pro, GarageBand, REAPER, FL Studio, Cubase, Studio One, Bitwig… ([full list](docs/DAWS.md)) |
| 🎸 | A guitar and an **audio interface** | Or no guitar at all: you can drop a recorded DI clip on a track |
| 💾 | About **3 GB** of free disk space | The JUCE framework and the build files |
| 🌐 | An internet connection | Only for the first build |
| ⏱️ | About **15 minutes** | Mostly waiting for the computer |

### 1️⃣ Open Terminal

Press <kbd>⌘ Cmd</kbd> + <kbd>Space</kbd>, type **Terminal**, press <kbd>Enter</kbd>.

> [!TIP]
> Every grey code box below is a command. Click the copy button in its top-right corner, paste it into Terminal with <kbd>⌘ Cmd</kbd> + <kbd>V</kbd>, then press <kbd>Enter</kbd>.

### 2️⃣ Install Apple's developer tools

```bash
xcode-select --install
```

A window pops up. Click **Install** and wait until it finishes.
If Terminal says *"already installed"*, you're done with this step. 🎉

### 3️⃣ Install Homebrew (skip if you already have it)

Check first:

```bash
brew --version
```

If you see a version number, skip to step 4. If you see *"command not found"*, install Homebrew:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

It asks for your Mac password (you won't see the characters while you type; that's normal).
At the end it prints **"Next steps"** with two or three commands. **Run those too**, so Terminal can find `brew`.

### 4️⃣ Install the build tools

```bash
brew install cmake ninja
```

### 5️⃣ Download the code

```bash
cd ~/Documents
git clone https://github.com/mitradeniz/FreeAudioFX.git
cd FreeAudioFX
```

<sub>Already have the folder? Just `cd` into it and run `git pull` to get the latest version.</sub>

### 6️⃣ Build the plugins

First, prepare the build. This downloads the JUCE framework, so it takes about a minute:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

✅ It worked if the last line says **`Build files have been written to: …`**

Now build all 45 plugins. Grab a coffee ☕, the first build takes about 5-20 minutes:

```bash
cmake --build build
```

You'll see a counter like `[57/250] Building CXX object …`. The JUCE framework is compiled once at the start, then each plugin is put together; long quiet stretches are normal, **let it finish.**

<sub>In a hurry? Build just one plugin, for example `cmake --build build --target DreamAmp_All` (all target names are in the [build guide](docs/BUILDING.md#build-just-one-plugin)).</sub>

✅ It worked if Terminal shows your prompt again **without** the word `FAILED`.

The build copies the plugins into your Mac's plug-in folders automatically. Check that they're there:

```bash
ls ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
```

You should see 45 `OAP … .vst3` plugins and 45 `OAP … .component` plugins, from `OAP 8-Bit Amp` to `OAP Wah`. 🎉

<details>
<summary>🎧 <b>Optional: try an amp without a DAW first</b></summary>
<br>

Every amp, the Pedalboard and the Looper are also built as standalone apps:

```bash
open "build/plugins/DriveAmp/DriveAmp_artefacts/Release/Standalone/OAP Drive Amp.app"
open "build/plugins/effectamps/DreamAmp_artefacts/Release/Standalone/OAP Dream Amp.app"
open "build/plugins/Pedalboard/Pedalboard_artefacts/Release/Standalone/OAP Pedalboard.app"
open "build/plugins/Looper/Looper_artefacts/Release/Standalone/OAP Looper.app"
```

Click **Options → Audio/MIDI Settings**, pick your audio interface, then unmute the input. **Use headphones** to avoid feedback.

</details>

<a id="step-7"></a>
### 7️⃣ Make your DAW find the plugins

Most DAWs scan for new plug-ins at start-up or have a **Rescan** button. Pick yours:

<details open>
<summary><b>Ableton Live</b> (macOS, Windows)</summary>
<br>

1. Open Live and go to **Settings** (<kbd>⌘ Cmd</kbd> + <kbd>,</kbd>) → **Plug-Ins** tab.
2. Turn **on**: **Use VST3 Plug-In System Folders** and, on a Mac, **Use Audio Units v2**.
3. Click **Rescan**.
4. In the browser on the left, click **Plug-Ins**. You'll find a folder called **Open Audio Plugins**, or just type **OAP** in the search box.

> [!TIP]
> On a Mac you'll see each plugin twice: once as an Audio Unit and once as a VST3. They sound identical, so pick either one.

</details>

<details>
<summary><b>Logic Pro / GarageBand / MainStage</b> (macOS)</summary>
<br>

They use the Audio Unit versions and scan them at launch.
**Logic Pro:** click an empty **Audio FX** slot → **Audio Units → Open Audio Plugins**. Missing? **Logic Pro → Settings → Plug-in Manager**, select it, **Reset & Rescan Selection**.
**GarageBand:** **GarageBand → Settings → Audio/MIDI** → tick **Enable Audio Units**, then in the track's **Smart Controls → Plug-ins**, click an empty slot → **Audio Units → Open Audio Plugins**.

</details>

<details>
<summary><b>REAPER, FL Studio, Cubase, Studio One, Bitwig and others</b></summary>
<br>

| DAW | Rescan |
| --- | --- |
| REAPER | **Options → Preferences → Plug-ins → VST → Re-scan** |
| FL Studio | **Options → Manage plugins → Find installed plugins** |
| Cubase / Nuendo | automatic at start-up (check **Studio → VST Plug-in Manager**) |
| Studio One | automatic at start-up |
| Bitwig Studio | automatic |

More DAWs, Windows and Linux folders: **[docs/DAWS.md](docs/DAWS.md)**.

</details>

### 8️⃣ Set up a guitar track

The idea is the same in every DAW. Here's Ableton Live as the example:

1. **Settings → Audio**: choose your audio interface as the **Audio Input Device** and **Audio Output Device**. Set **Buffer Size** to **128** samples (lower means less delay).
2. Create an audio track: <kbd>⌘ Cmd</kbd> + <kbd>T</kbd>.
3. In the track's **Audio From**, choose **Ext. In**, then the single input your guitar is plugged into (for example **1**, not **1/2**).
4. Set **Monitor** to **In**.
5. Drag an amp (say **OAP Drive Amp** or **OAP Dream Amp**) from the browser onto the track. Want pedals? Drop them on the track **before** the amp, or use the **OAP Pedalboard**.
6. Play! 🤘

<sub>Other DAWs: create a mono audio track with your guitar input, turn on input monitoring, and insert the plugins as track effects. Details in [docs/DAWS.md](docs/DAWS.md#setting-up-a-guitar-track-any-daw).</sub>

> [!IMPORTANT]
> Set your interface's **input gain** so hard strums light its meter green or yellow, **never red**. Too little gain makes the amps sound thin; too much makes them noisy.

### 9️⃣ Have fun

- **Double-click** any knob to reset it.
- Hold **Shift** while dragging for fine adjustments.
- Drag the bottom-right corner to make the window bigger or smaller.
- Try the [tone recipes](#tone-recipes) below as starting points.

---

<a id="tone-recipes"></a>
## 🍳 Tone recipes

Starting points to tweak by ear. Your guitar, pickups and playing will change everything, and that's the fun part.

| Recipe | Amp | Settings |
| --- | --- | --- |
| 🤘 **Modern metal chug** | Drive | Boost **on** · Tight 7 · Gain 6 · Bass 5 · Middle 6 · Treble 6.5 · Presence 6 · Resonance 5 · Master 5 · Gate -60 dB · 4x12 |
| 🎸 **Classic rock crunch** | Drive | Boost off · Tight 4 · Gain 2.5 · Bass 5 · Middle 7 · Treble 6 · Presence 5 · Master 6 · 4x12 |
| 🚀 **Singing lead** | Drive | Boost **on** · Tight 5 · Gain 8 · Middle 7 · Treble 5.5 · Presence 4 · Master 6 · 2x12 |
| 💎 **Glassy clean** | Clean | Volume 3.5 · Bright **on** · Bass 4 · Middle 5 · Treble 7 · Reverb 4 · Master 4 · 2x12 |
| 🎷 **Blues on the edge** | Clean | Volume 7.5 · Bright off · Bass 5 · Middle 6 · Treble 5.5 · Reverb 2 · Master 7 · 1x12 |
| 🕺 **Funk spank** | Clean | Volume 4 · Bright **on** · Bass 3 · Middle 4 · Treble 7.5 · Reverb 1 · Master 5 · 1x12 |
| 🌌 **Ambient swell** | Dream Amp | Gain 3 · Treble 7 · Chorus 5 · Echo 5 · Shimmer 7 · Space 9 (roll your volume knob in) |
| 🌴 **Dub skank** | Dub Amp | Bass 2 · Middle 6 · Treble 7 · Echo Time 375 ms · Feedback 6 · Echo 5 (crank Feedback at the end of a phrase) |
| 🖤 **Tremolo-picked riffs** | Black Metal Amp | Gate -55 dB · Gain 8 · Bass 4 · Middle 3 · Treble 7 · Hall 5 · Decay 8 |
| 🏄 **Surf twang** | Surf Amp | Bass 3 · Treble 8 · Dwell 8 · Reverb 7 · Intensity 0 |
| ⚡ **Galloping riffs** | Heavy Metal Amp | Gate -55 dB · Gain 7 · Boost 5 · Middle 7 · Treble 6.5 · Delay 0 · Reverb 2 |
| 🛡️ **Dragon-slaying solo** | Knight Amp | Gain 7 · Harmony 5 · Interval 5th · Echo 5 · Hall 5 |
| 👾 **Boss level** | 8-Bit Amp | Chip 8 · Bits 6 · Crush 5 · Echo 4 (single notes, high on the neck) |
| 🪩 **Disco strut** | Funk Amp | Squeeze 6 · Quack 7 · Range 3 · Phase 2 (muted sixteenths) |
| 🌸 **City-pop chords** | Sakura Amp | Gain 4 · Treble 7 · Squeeze 5 · Chorus 5 · Echo 3 · Plate 4 |
| 🤠 **Desert showdown** | Western Amp | Bass 6 · Treble 7 · Slapback 5 · Tremolo 4 · Speed 5 Hz · Spring 5 |
| ⛪ **Cathedral swell** | Choir Amp | Gain 3 · Choir 8 · Voices Oct+5th · Cathedral 8 (swell with your volume knob) |

**Pedal chains** (put the pedals before an amp, in this order, or load them into the Pedalboard):

| Recipe | Chain |
| --- | --- |
| 🎛️ **Tight metal** | Boost (Gain 6 dB, Bass -6 dB) → Drive Amp (Boost off) |
| 🔊 **Blues lead** | Compressor (Sustain 4, Blend 60 %) → Overdrive (Drive 4, Level 6) → Clean Amp (Volume 6) → Delay (Analog, 1/4, Mix 25 %) |
| 🌊 **Post-rock wall** | Fuzz → Clean Amp → Tape Echo (Heads 1+2+3) → Shimmer (Mix 50 %) |
| 🎹 **Synth lead** | Guitar Synth (Sub 60 %, Sweep 7) → Phaser → Drive Amp (Gain 3) |
| 🧊 **Pad under a solo** | Freeze (step on it over a chord) → Reverb (Hall) → play the solo on top |

### 🔈 Use your own cabinet (impulse responses)

Have a favourite cab IR? Click **Load IR…** under the amp and pick a `.wav`, `.aiff` or `.flac` file.
Its volume is matched to the built-in cabs automatically, and the file is remembered with your project.
This works on every amp and effect amp.
Want to use a separate cabinet plugin instead? Set **Cabinet** to **Off**.

---

<a id="troubleshooting"></a>
## 🩺 Troubleshooting

<details>
<summary><b>The plugins don't show up in my DAW</b></summary>
<br>

1. Check that the build finished without `FAILED` (step 6).
2. Check the plugin folder. **macOS:** `ls ~/Library/Audio/Plug-Ins/VST3`. **Windows:** `C:\Program Files\Common Files\VST3`. **Linux:** `ls ~/.vst3`. Do you see `OAP Drive Amp.vst3`? On Windows and Linux you copy them there yourself ([quick start](#quick-start)).
3. Rescan in your DAW ([steps for each DAW](docs/DAWS.md)). In **Ableton Live**: **Settings → Plug-Ins**, make sure **Use VST3 Plug-In System Folders** and/or **Use Audio Units v2** is on, then click **Rescan**; hold <kbd>⌥ Option</kbd> while clicking it to force a full rescan.
4. **Logic Pro / GarageBand** only see the Audio Unit (`.component`) versions. **Pro Tools** isn't supported yet (it needs AAX).

</details>

<details>
<summary><b>I don't hear anything</b></summary>
<br>

- Is input monitoring on for the track (Ableton: **Monitor** set to **In**)?
- Is the track's input set to the input your guitar is actually plugged into?
- Is the track's meter moving? If not, the problem is before the amp: check the interface and its input gain.
- Did you turn **Output** on the amp all the way down?

</details>

<details>
<summary><b>I hear my guitar twice, or a clean sound under the amp</b></summary>
<br>

Your audio interface is probably sending the dry guitar straight to your headphones ("direct monitoring"). Turn direct monitoring off on the interface or in its control app.

</details>

<details>
<summary><b>There's a delay between picking and hearing</b></summary>
<br>

Lower your DAW's **Buffer Size** (in its audio settings) to 64 or 128. The plugins themselves add at most about 1 ms, and report it to your DAW.

</details>

<details>
<summary><b>It hisses or hums when I stop playing</b></summary>
<br>

Turn up the **Gate** (Drive Amp, Black Metal, Arena, Doom, Knight and Heavy Metal Amp) until the noise disappears between notes. Single-coil pickups hum near screens. Try turning away from your computer.

</details>

<details>
<summary><b>Crackles and pops</b></summary>
<br>

Raise the **Buffer Size** (256), and close other heavy apps. Make sure you built with `-DCMAKE_BUILD_TYPE=Release` (on Windows: `--config Release`).

</details>

<details>
<summary><b>The Looper waits instead of recording, or stops recording by itself</b></summary>
<br>

That is **Sync** at work. While your DAW is playing (or the Looper's **Click** is on), recording starts and ends on the next bar line, so the big button blinks until then: set **Quantize** to *Off* to act at once, or switch **Sync** off for a plain looper. If the recording ends by itself, **Length** is set to a fixed number of beats or bars: choose *Free*. And with **Auto Start** on, the Looper waits for your first note. 📖 [In time with your project](docs/plugins/Looper.md#in-time-with-your-project)

</details>

<details>
<summary><b>The Looper records silence</b></summary>
<br>

Watch the **IN** meter at the bottom of the Looper's display while you play. If it doesn't move, no signal reaches the Looper: check the track's input and monitoring (see *I don't hear anything* above), and that the Looper sits **after** the amp on the same track.

</details>

<details>
<summary><b>The build failed</b></summary>
<br>

- `cmake: command not found`: run step 4 again, then open a **new** Terminal window.
- `xcrun: error: invalid active developer path`: run step 2 (`xcode-select --install`).
- Windows: build from a *Developer PowerShell for VS 2022* so CMake finds the compiler.
- Linux: `Package … not found` or missing headers: install the packages from the [quick start](#quick-start).
- Anything else: scroll up to the first line containing `error:`, copy it together with a few lines around it, and [open an issue](https://github.com/mitradeniz/FreeAudioFX/issues).

</details>

<details>
<summary><b>How do I update or uninstall?</b></summary>
<br>

**Update:**

```bash
cd ~/Documents/FreeAudioFX
git pull
cmake --build build
```

Then rescan in your DAW (on Windows and Linux, copy the plugins again first).

**Uninstall:** delete the `OAP …` files from your plug-in folders. macOS: `~/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/Components`. Windows: `C:\Program Files\Common Files\VST3`. Linux: `~/.vst3`.

</details>

<details>
<summary><b>An effect amp's echo keeps going forever</b></summary>
<br>

That's the tape echo self-oscillating (Dub and Space Delay Amp): turn **Feedback** / **Intensity** down and it dies away. It's a feature. 🛸

</details>

---

## 🔬 What's inside

```mermaid
flowchart LR
    G([🎸 Guitar]) --> GATE[Gate] --> TIGHT[Tight] --> BOOST[Boost]
    subgraph OS["8x oversampled"]
        PRE["4 tube stages"] --> TS["Tone stack"] --> PWR["Presence / Resonance<br/>Power amp + sag"]
    end
    BOOST --> PRE
    PWR --> CAB["Cabinet<br/>4x12 · 2x12 · 1x12 · IR"] --> OUT([🔊 Output])
```

<sub>The OAP Drive Amp signal path. The Clean Amp follows the same idea: Bright, two tube stages, tone stack, Volume, power amp, cabinet and reverb.</sub>

```mermaid
flowchart LR
    G([🎸 Guitar]) --> GATE["Gate<br/>(some amps)"] --> FUZZ["Pedals<br/>(fuzz, boost, synth, compressor…)"]
    subgraph OS["4x / 8x oversampled"]
        PRE["1-4 tube stages"] --> TS["Tone stack"] --> PWR["Power amp + sag"]
    end
    FUZZ --> PRE
    PWR --> CAB["Cabinet / IR"] --> FX["Stereo effects<br/>chorus · delay · reverb…"] --> OUT([🔊 Output])
```

<sub>Every effect amp: one shared engine, configured by a short description of the amp in <a href="shared/amps/AmpModels.cpp"><code>AmpModels.cpp</code></a>, plus an SVG scene drawn by <a href="tools/art/make_amp_art.py"><code>make_amp_art.py</code></a>.</sub>

- **Mono amps, like real ones.** Stereo input is summed and the amp drives both speakers; reverbs, ping-pong delays and stereo choruses then spread it out.
- **Low latency:** 6 samples for the amps, up to about 1 ms for the oversampled drive pedals, always reported to your DAW for compensation.
- **Light on the CPU** (one core at 48 kHz, measured on an x86 machine): amps 1.5-5 %, effect amps 2-8 %, pedals 0.05-3.5 %.
- **A looper that never makes the audio wait:** the loop's memory is prepared ahead of the recording by another thread, undo copies only what an overdub touches, and saving, loading and resizing the loop talk to the audio thread without locks.
- **Tested:** about 1,600 automated checks. DSP: stability at 44.1-192 kHz with random automation, silence, aliasing, levels, impulse responses, every effect amp knob. Looper: sample-exact playback, undo, tempo sync, the auto loop and the rhythms, and other threads working on the loop while audio runs (also under thread and memory sanitizers). Plugin layer: the Looper's session state, audio files, host tempo, auto loop and mapped switches. And every plugin passes [pluginval](https://github.com/Tracktion/pluginval) at its strictest level.

Want to dig deeper? 📚 [Plugin manuals](docs/plugins/) · [Using the plugins in your DAW](docs/DAWS.md) · [Building on every platform](docs/BUILDING.md) · [Developer guide](docs/DEVELOPMENT.md)

### Project structure

```text
FreeAudioFX/
├── plugins/
│   ├── DriveAmp/ CleanAmp/   # the two amps
│   ├── effectamps/           # 21 effect amps, one shared source file
│   ├── pedals/               # 20 pedals, one shared source file
│   ├── Pedalboard/
│   └── Looper/
├── shared/
│   ├── dsp/                  # amp building blocks: tubes, tone stacks, cabinets, gate
│   ├── fx/                   # the effects library behind every pedal
│   ├── amps/                 # effect amp models and engine
│   ├── looper/               # the looper engine
│   ├── art/                  # the artwork of every amp (SVG)
│   ├── plugin/ ui/           # plugin wrappers, knobs, editors
├── tests/                    # automated DSP and plugin tests
├── tools/                    # artwork generator and editor snapshot renderer
├── docs/                     # manuals, DAW guide, build guide, developer guide, artwork
├── cmake/                    # shared CMake helpers
└── CMakeLists.txt            # top-level build (downloads JUCE automatically)
```

### Built with

**C++17** for the real-time DSP · **[JUCE 9](https://juce.com)** for the plugin formats and UI · **CMake** for reproducible builds · **VST3 / AU** as distribution targets.
Python is used only for side jobs such as generating the README artwork, never inside the plugins.

---

<a id="roadmap"></a>
## 🗺️ Roadmap

- [x] Set up the C++ / JUCE / CMake project template.
- [x] Build the first audio effects (Drive Amp, Clean Amp) and validate them with pluginval.
- [x] Add automated DSP tests.
- [x] 20 pedals (12 everyday, 8 advanced) and a Pedalboard.
- [x] 21 effect amps.
- [x] Artwork for every amp, with ornaments around the controls.
- [x] Looper: overdub with undo, tempo sync, and loops saved with the project.
- [ ] Hands-on testing in Ableton Live (incl. Lite), Logic Pro, GarageBand, REAPER, FL Studio, Cubase, Studio One and Bitwig on macOS and Windows, and REAPER/Bitwig/Ardour on Linux.
- [ ] Presets and an input level meter.
- [ ] Pro Tools (AAX) and CLAP builds.
- [ ] Publish the first versioned release with downloadable builds.
- [ ] Expand the collection:
  - 🎚️ **Mixing effects:** EQ, filters and dynamics
  - 🌀 **Creative sound design:** modulation, experimental effects, workflow utilities
  - 🎹 **Virtual instruments:** simple synthesizers and other sound generators
  - 📈 **Audio analysis:** meters, visualizers and diagnostic tools
  - 🤖 **Optional AI-assisted tools**, only where they genuinely help

These are planned areas, not promises of finished features. Each plugin gets its own documentation and release notes when it's ready.

## 🧭 Development principles

1. **Free to use.** Released plugins are intended to be available at no cost.
2. **Transparent.** Features, limitations and dependencies are documented.
3. **Efficient.** Low latency and predictable CPU use come first.
4. **DAW-friendly.** Compatibility, presets and automation are tested before release.
5. **Easy to maintain.** Code and documentation stay organised and approachable.

## 📦 Downloads

No binaries are published yet. Once a plugin is ready, builds and installation instructions will appear in this repository's **Releases** section. Please avoid unofficial download mirrors.

## ⚖️ License

A source-code license has **not been selected yet**. Public visibility does not automatically grant permission to reuse, modify or redistribute the code. Licensing information will be added before the first public source release. Third-party dependencies keep their own licenses.

> [!NOTE]
> JUCE is dual-licensed under the AGPLv3 and a commercial JUCE licence. Releasing these plugins under terms other than the AGPLv3 (for example, closed source) may require a commercial JUCE licence. See the [licensing note](docs/BUILDING.md#licensing-note).

## 👤 Author

**Mitra Deniz Karataş**

If you build a tone you love, or something breaks, [open an issue](https://github.com/mitradeniz/FreeAudioFX/issues). Feedback makes these plugins better. 🎸

---

<sub>Open Audio Plugins is an independent project and is not affiliated with or endorsed by any DAW, plug-in format or amplifier vendor. Product names and trademarks belong to their respective owners. The built-in cabinets and amp voicings are original approximations, not captures of specific commercial products.</sub>
