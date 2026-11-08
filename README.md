<p align="center">
  <img src="docs/assets/hero.svg" width="100%" alt="Open Audio Plugins: free guitar amps and effects for your DAW">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/status-early%20development-ff5a1f?style=flat-square" alt="Status: early development">
  <img src="https://img.shields.io/badge/formats-VST3%20%7C%20AU%20%7C%20Standalone-19b3a3?style=flat-square" alt="Formats: VST3, AU, Standalone">
  <img src="https://img.shields.io/badge/macOS%20%7C%20Windows%20%7C%20Linux-555?style=flat-square" alt="Platforms: macOS, Windows, Linux">
  <img src="https://img.shields.io/badge/built%20with-JUCE%209%20%C2%B7%20C%2B%2B17-555?style=flat-square" alt="Built with JUCE 9 and C++17">
  <img src="https://img.shields.io/badge/price-free-2ea44f?style=flat-square" alt="Price: free">
</p>

<p align="center">
  <b>Plug in your guitar, open your DAW, turn some knobs.</b><br>
  Open Audio Plugins is a growing collection of free, open audio plugins: practical tools with clear interfaces,<br>
  reliable performance and no unnecessary restrictions. The first two are guitar amps. 🎸
</p>

<p align="center">
  <a href="#the-amps">The amps</a> ·
  <a href="#quick-start">Quick start</a> ·
  <a href="#step-by-step">Step-by-step guide</a> ·
  <a href="#tone-recipes">Tone recipes</a> ·
  <a href="#troubleshooting">Troubleshooting</a> ·
  <a href="#roadmap">Roadmap</a>
</p>

> [!NOTE]
> **Early development.** There are no downloadable builds yet, so for now you build the plugins yourself.
> Don't worry: it's copy-paste, about 10 minutes, mostly waiting. The [step-by-step guide](#step-by-step) covers every click.

---

<a id="the-amps"></a>
## 🎛️ Meet the amps

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

<details>
<summary><b>Which amp should I pick?</b></summary>
<br>

| You want… | Pick | Start with |
| --- | --- | --- |
| Metal, djent, hard rock | 🔥 Drive Amp | the defaults, then **Boost** on |
| Classic rock crunch | 🔥 Drive Amp | **Gain** 2-3 |
| Funk, pop, jazz, indie cleans | 💎 Clean Amp | the defaults |
| Blues, "just breaking up" | 💎 Clean Amp | **Volume** 7-8 |

</details>

---

<a id="quick-start"></a>
## ⚡ Quick start (Mac, if you've done this before)

<p align="center">
  <img src="docs/assets/terminal.svg" width="100%" alt="Terminal animation: installing CMake and Ninja, cloning the repository, configuring and building the plugins">
</p>

```bash
brew install cmake ninja
git clone https://github.com/mitradeniz/FreeAudioFX.git
cd FreeAudioFX
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Then in Ableton Live: **Settings → Plug-Ins → Rescan**. Search for **OAP**. Rock. 🤘

Never used Terminal? No problem, read on. 👇

---

<a id="step-by-step"></a>
## 🪜 Step-by-step guide (macOS + Ableton Live)

<p align="center">
  <img src="docs/assets/signal-flow.svg" width="100%" alt="Guitar into audio interface, into an Ableton Live audio track, through the OAP amp, out to headphones">
</p>

### ✅ Step 0: Check what you need

| | You need | Notes |
| --- | --- | --- |
| 💻 | A Mac with **macOS 12** or newer | Apple Silicon or Intel |
| 🎹 | **Ableton Live 12** | Lite, Intro, Standard and Suite all work with plug-ins |
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

Now build. Grab a coffee ☕, this takes 5-15 minutes:

```bash
cmake --build build
```

You'll see a counter like `[57/198] Building CXX object …`. Near the end it may sit quiet on the last steps for a few minutes while it optimises the code. **That's normal, let it finish.**

✅ It worked if Terminal shows your prompt again **without** the word `FAILED`.

The build copies the plugins into your Mac's plug-in folders automatically. Check that they're there:

```bash
ls ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
```

You should see `OAP Drive Amp.vst3`, `OAP Clean Amp.vst3`, `OAP Drive Amp.component` and `OAP Clean Amp.component`. 🎉

<details>
<summary>🎧 <b>Optional: try an amp without a DAW first</b></summary>
<br>

Each amp is also built as a standalone app:

```bash
open "build/plugins/DriveAmp/DriveAmp_artefacts/Release/Standalone/OAP Drive Amp.app"
```

Click **Options → Audio/MIDI Settings**, pick your audio interface, then unmute the input. **Use headphones** to avoid feedback.

</details>

### 7️⃣ Make Ableton Live find the plugins

1. Open Live and go to **Settings** (<kbd>⌘ Cmd</kbd> + <kbd>,</kbd>) → **Plug-Ins** tab.
2. Turn **on**: **Use Audio Units v2** and **Use VST3 Plug-In System Folders**.
3. Click **Rescan**.
4. In the browser on the left, click **Plug-Ins**. You'll find a folder called **Open Audio Plugins**, or just type **OAP** in the search box.

> [!TIP]
> You'll see each amp twice: once as an Audio Unit and once as a VST3. They sound identical, so pick either one.

### 8️⃣ Set up a guitar track

1. **Settings → Audio**: choose your audio interface as the **Audio Input Device** and **Audio Output Device**. Set **Buffer Size** to **128** samples (lower means less delay).
2. Create an audio track: <kbd>⌘ Cmd</kbd> + <kbd>T</kbd>.
3. In the track's **Audio From**, choose **Ext. In**, then the single input your guitar is plugged into (for example **1**, not **1/2**).
4. Set **Monitor** to **In**.
5. Drag **OAP Drive Amp** or **OAP Clean Amp** from the browser onto the track.
6. Play! 🤘

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

### 🔈 Use your own cabinet (impulse responses)

Have a favourite cab IR? Click **Load IR…** under the amp and pick a `.wav`, `.aiff` or `.flac` file.
Its volume is matched to the built-in cabs automatically, and the file is remembered with your Live set.
Want to use a separate cabinet plugin instead? Set **Cabinet** to **Off**.

---

<a id="troubleshooting"></a>
## 🩺 Troubleshooting

<details>
<summary><b>The amps don't show up in Ableton Live</b></summary>
<br>

1. Check that the build finished without `FAILED` (step 6).
2. Run `ls ~/Library/Audio/Plug-Ins/VST3`. Do you see `OAP Drive Amp.vst3`?
3. In Live: **Settings → Plug-Ins**, make sure **Use VST3 Plug-In System Folders** and/or **Use Audio Units v2** is on, then click **Rescan**.
4. Still nothing? Hold <kbd>⌥ Option</kbd> while clicking **Rescan** to force a full rescan.

</details>

<details>
<summary><b>I don't hear anything</b></summary>
<br>

- Is the track's **Monitor** set to **In**?
- Is **Audio From** set to the input your guitar is actually plugged into?
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

Lower the **Buffer Size** in **Settings → Audio** to 64 or 128. The amps themselves add only about 0.1 ms.

</details>

<details>
<summary><b>It hisses or hums when I stop playing</b></summary>
<br>

Turn up the Drive Amp's **Gate** until the noise disappears between notes. Single-coil pickups hum near screens. Try turning away from your computer.

</details>

<details>
<summary><b>Crackles and pops</b></summary>
<br>

Raise the **Buffer Size** (256), and close other heavy apps. Make sure you built with `-DCMAKE_BUILD_TYPE=Release`.

</details>

<details>
<summary><b>The build failed</b></summary>
<br>

- `cmake: command not found`: run step 4 again, then open a **new** Terminal window.
- `xcrun: error: invalid active developer path`: run step 2 (`xcode-select --install`).
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

Then **Rescan** in Live.

**Uninstall:** delete the `OAP …` files from `~/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/Components`.

</details>

<details>
<summary><b>🪟 Windows and 🐧 Linux</b></summary>
<br>

Both are supported (VST3 and Standalone). See **[docs/BUILDING.md](docs/BUILDING.md)** for the steps.
On Windows you copy the `.vst3` files into `C:\Program Files\Common Files\VST3\` yourself.

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

- **Mono amp, like a real one.** Stereo input is summed, and the result goes to both speakers (the Clean Amp's reverb spreads it into stereo).
- **Low latency:** about 6 samples, reported to your DAW for compensation.
- **Light on the CPU:** about 4 % (Drive) and 1.5 % (Clean) of one core at 48 kHz, measured on an x86 machine.
- **Tested:** 173 automated DSP checks (stability at 44.1-192 kHz, silence, aliasing, levels, impulse responses) and [pluginval](https://github.com/Tracktion/pluginval) at its strictest level.

Want to dig deeper? 📚 [Plugin manuals](docs/plugins/) · [Building on every platform](docs/BUILDING.md) · [Developer guide](docs/DEVELOPMENT.md)

### Project structure

```text
FreeAudioFX/
├── plugins/        # one folder per plugin (DriveAmp, CleanAmp)
├── shared/         # reusable DSP, UI and plugin building blocks
├── tests/          # automated DSP tests
├── docs/           # manuals, build guide, developer guide, artwork
├── cmake/          # shared CMake helpers
└── CMakeLists.txt  # top-level build (downloads JUCE automatically)
```

### Built with

**C++17** for the real-time DSP · **[JUCE 9](https://juce.com)** for the plugin formats and UI · **CMake** for reproducible builds · **VST3 / AU** as distribution targets.
Python is used only for side jobs such as generating the README artwork, never inside the plugins.

---

<a id="roadmap"></a>
## 🗺️ Roadmap

- [x] Set up the C++ / JUCE / CMake project template.
- [x] Build the first audio effects (Drive Amp, Clean Amp) and validate them with pluginval.
- [x] Add basic automated DSP tests.
- [ ] Verify Ableton Live 12 Lite compatibility.
- [ ] Test the macOS AU builds and other compatible DAWs.
- [ ] Presets and an input level meter.
- [ ] Publish the first versioned release with downloadable builds.
- [ ] Expand the collection:
  - 🎚️ **Audio effects:** EQ, filters, distortion, delay, reverb and dynamics
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

<sub>Open Audio Plugins is an independent project and is not affiliated with or endorsed by Ableton or any other DAW or amplifier vendor. Product names and trademarks belong to their respective owners. The built-in cabinets and amp voicings are original approximations, not captures of specific commercial products.</sub>
