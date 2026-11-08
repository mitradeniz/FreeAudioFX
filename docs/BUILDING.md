# Building the plugins

The project uses **CMake** and **JUCE 9.0.5**. JUCE is downloaded automatically
the first time you configure, so the first configure needs an internet
connection and takes a minute or two.

One build produces all 33 plugins: 2 amps, 10 effect amps, 20 pedals and the
Pedalboard, as VST3 everywhere, Audio Units on macOS, and standalone apps for
the amps and the Pedalboard. Once built, see [DAWS.md](DAWS.md) for loading
them in your DAW.

| Platform | Formats | Compiler |
| --- | --- | --- |
| macOS 12+ (Apple Silicon or Intel) | VST3, AU, Standalone | Xcode / Command Line Tools |
| Windows 10/11 (x64) | VST3, Standalone | Visual Studio 2022 |
| Linux (x64; e.g. Ubuntu 22.04+) | VST3, Standalone | GCC 11+ or Clang 14+ |

## macOS (VST3, AU, Standalone)

### 1. Install the tools

- **Xcode** from the App Store (or just the Command Line Tools:
  `xcode-select --install`).
- **CMake 3.22+** and, optionally, **Ninja** (faster builds):

  ```bash
  brew install cmake ninja
  ```

### 2. Configure and build

From the repository root:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Prefer Xcode? Generate a project instead and open `build/OpenAudioPlugins.xcodeproj`:

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

The JUCE framework is compiled only once and shared by every plugin in the
collection, so the first full build takes about 5-20 minutes depending on your
machine, and later builds only recompile what changed. For quicker
edit-and-test cycles use `-DCMAKE_BUILD_TYPE=RelWithDebInfo`.

### Build just one plugin

Every plugin is a CMake target. Add `_VST3`, `_AU` or `_Standalone` for one
format, or `_All` for all of them:

```bash
cmake --build build --target DriveAmp_VST3        # just the Drive Amp VST3
cmake --build build --target DreamAmp_AU          # just the Dream Amp Audio Unit
cmake --build build --target Pedalboard_Standalone
cmake --build build --target OverdrivePedal_All   # every format of one plugin
```

| Group | Target names |
| --- | --- |
| Amps | `DriveAmp`, `CleanAmp` |
| Effect amps | `DreamAmp`, `DubAmp`, `SpaceDelayAmp`, `BlackMetalAmp`, `WarmRockAmp`, `SurfAmp`, `ShoegazeAmp`, `ArenaAmp`, `StereoJazzAmp`, `DoomAmp` |
| Pedals | `CompressorPedal`, `BoostPedal`, `OverdrivePedal`, `DistortionPedal`, `FuzzPedal`, `ChorusPedal`, `FlangerPedal`, `PhaserPedal`, `TremoloPedal`, `DelayPedal`, `ReverbPedal`, `WahPedal`, `ShimmerPedal`, `TapeEchoPedal`, `PitchPedal`, `EnvFilterPedal`, `RotaryPedal`, `FreezePedal`, `SynthPedal`, `LoFiPedal` |
| Pedalboard | `Pedalboard` |

### 3. Where the plugins end up

With the default `OAP_COPY_PLUGINS=ON` (macOS), every build also copies the
plugins into your user plug-in folders:

| Format | Location |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/OAP <Name>.vst3` (for example `OAP Drive Amp.vst3`, `OAP Overdrive.vst3`) |
| AU | `~/Library/Audio/Plug-Ins/Components/OAP <Name>.component` |

The build products themselves are under `build/plugins/`:

| Plugins | Folder |
| --- | --- |
| Drive Amp, Clean Amp, Pedalboard | `build/plugins/<Target>/<Target>_artefacts/Release/` |
| Effect amps | `build/plugins/effectamps/<Target>_artefacts/Release/` |
| Pedals | `build/plugins/pedals/<Target>_artefacts/Release/` |

Each has `VST3/`, `AU/` and, for the amps and the Pedalboard, `Standalone/`.
The **Standalone** apps are handy for a quick test with your audio interface,
without opening a DAW.

To build Intel + Apple Silicon universal binaries, add
`-DOAP_UNIVERSAL_BINARY=ON` to the first configure command.

### 4. Check the Audio Units (optional)

Every plugin has the manufacturer code `Oapl` and its own plugin code:

```bash
auval -v aufx Drva Oapl   # OAP Drive Amp

# or all of them:
for code in Drva Clna Drem Dubx Spdl Blkm Wrmr Surf Shgz Aren Stjz Doom \
            Comp Bost Ovdr Dist Fuzz Chor Flng Phsr Trem Dlay Revb Wahx \
            Shim Tape Ptch Envf Rota Frez Synt Lofi Pdbd; do
  auval -v aufx $code Oapl | tail -1
done
```

## Loading the plugins in a DAW

See **[DAWS.md](DAWS.md)**: formats and folders per platform, rescan steps for
Ableton Live, Logic Pro, GarageBand, REAPER, FL Studio, Cubase, Studio One,
Bitwig Studio and more, and how to set up a guitar track.

## Windows (VST3, Standalone)

1. Install **Visual Studio 2022** with the *Desktop development with C++*
   workload, and **CMake 3.22+**.
2. From a *Developer Command Prompt*:

   ```bat
   cmake -B build
   cmake --build build --config Release
   ```

3. Copy the plugins into the system VST3 folder. Plug-in copying is off by
   default on Windows because that folder needs administrator rights, so run
   this in a PowerShell window opened with **Run as administrator**, from the
   repository folder:

   ```powershell
   Get-ChildItem build\plugins\*\*_artefacts\Release\VST3\*.vst3 |
       Copy-Item -Destination "C:\Program Files\Common Files\VST3" -Recurse -Force
   ```

   (Or copy the `.vst3` folders from `build\plugins\…\Release\VST3\` by hand.)
4. Rescan in your DAW ([DAWS.md](DAWS.md)). The standalone apps are the `.exe`
   files under `…\Release\Standalone\`.

## Linux (VST3, Standalone)

1. Install a compiler, CMake, Ninja and the JUCE dependencies. On Debian or
   Ubuntu:

   ```bash
   sudo apt install build-essential cmake ninja-build pkg-config git \
       libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev \
       libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
       libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev
   ```

   Other distributions: see
   [JUCE's Linux dependency guide](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md).
2. Build exactly as on macOS:

   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

3. Copy the plugins to your VST3 folder and rescan in your DAW (REAPER, Bitwig
   Studio, Ardour, Carla…):

   ```bash
   mkdir -p ~/.vst3
   cp -r build/plugins/*/*_artefacts/Release/VST3/*.vst3 ~/.vst3/
   ```

## Build options

| Option | Default | Meaning |
| --- | --- | --- |
| `OAP_BUILD_PLUGINS` | `ON` | Build the plugins. |
| `OAP_BUILD_TESTS` | `ON` | Build the DSP test runner. |
| `OAP_COPY_PLUGINS` | `ON` on macOS | Copy plugins to the plug-in folders after building (macOS: your user folders; Linux: `~/.vst3`; on Windows the folder needs administrator rights). |
| `OAP_UNIVERSAL_BINARY` | `OFF` | macOS: build for arm64 and x86_64. |
| `OAP_SHARED_JUCE` | `ON` | Compile JUCE once for all plugins. Set to `OFF` to give every plugin its own copy (slower; the stock JUCE setup). |
| `OAP_JUCE_PATH` | *(empty)* | Use a local JUCE checkout instead of downloading it. |

## Running the DSP tests

```bash
cmake --build build --target oap_dsp_tests
./build/tests/oap_dsp_tests_artefacts/Release/oap_dsp_tests
```

About a thousand checks run in a minute or so: every amp, effect amp and
pedal at several sample rates and block sizes with random automation, decay to
silence, output levels, aliasing, cabinets and impulse responses, gates, and
CPU use. Add `--render demos` to also write example WAV files (a synthetic
guitar played through the Drive and Clean amps) into a `demos/` folder. With
CTest: `ctest --test-dir build`.

The `oap_snapshots` tool renders the editor of every pedal, effect amp and the
Pedalboard to PNG files (used for the documentation images):

```bash
cmake --build build --target oap_snapshots
./build/tests/oap_snapshots_artefacts/Release/oap_snapshots snapshots
```

## Licensing note

JUCE is dual-licensed under the **AGPLv3** and a **commercial JUCE licence**.
If this project is distributed under the AGPLv3 (or a compatible licence), the
free JUCE licence applies. Releasing the plugins under any other terms - for
example closed source - may require a commercial JUCE licence. Check
[JUCE's licensing page](https://juce.com/legal/juce-9-licence/) before choosing
the project licence.
