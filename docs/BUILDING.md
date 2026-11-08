# Building the plugins

The project uses **CMake** and **JUCE 9.0.5**. JUCE is downloaded automatically
the first time you configure, so the first configure needs an internet
connection and takes a minute or two.

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

A Release build uses link-time optimisation, so the final link step can take a
few minutes. For quicker edit-and-test cycles use `-DCMAKE_BUILD_TYPE=RelWithDebInfo`.

### 3. Where the plugins end up

With the default `OAP_COPY_PLUGINS=ON` (macOS), every build also copies the
plugins into your user plug-in folders:

| Format | Location |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/OAP Drive Amp.vst3`, `OAP Clean Amp.vst3` |
| AU | `~/Library/Audio/Plug-Ins/Components/OAP Drive Amp.component`, `OAP Clean Amp.component` |

The build products themselves are under
`build/plugins/<Plugin>/<Plugin>_artefacts/Release/` (`VST3/`, `AU/`, `Standalone/`).
The **Standalone** apps are handy for a quick test with your audio interface,
without opening a DAW.

To build Intel + Apple Silicon universal binaries, add
`-DOAP_UNIVERSAL_BINARY=ON` to the first configure command.

### 4. Check the Audio Units (optional)

```bash
auval -v aufx Drva Oapl   # OAP Drive Amp
auval -v aufx Clna Oapl   # OAP Clean Amp
```

## Using the plugins in Ableton Live 12 Lite

1. Open **Live > Settings > Plug-Ins** and enable the VST3 system folder and/or
   Audio Units, then press **Rescan**.
2. The amps appear in the browser under **Plug-Ins** as *OAP Drive Amp* and
   *OAP Clean Amp* (manufacturer: Open Audio Plugins).
3. For a guitar plugged into an audio interface:
   - create an audio track, set **Audio From** to the interface input your
     guitar is on (a single mono input such as "1"),
   - set **Monitor** to **In**,
   - drop an amp on the track.
4. Use a small buffer (64-128 samples, in **Settings > Audio**) for a tight feel.
   The amps themselves add only 6 samples of latency, which they report to Live
   for compensation.

The amps are mono internally, like real amps. Stereo input is summed to mono
and the result goes to both output channels (the Clean Amp's reverb then
spreads it into stereo).

## Windows (VST3, Standalone)

1. Install **Visual Studio 2022** with the *Desktop development with C++*
   workload, and **CMake 3.22+**.
2. From a *Developer Command Prompt*:

   ```bat
   cmake -B build
   cmake --build build --config Release
   ```

3. Copy the `.vst3` folders from
   `build\plugins\<Plugin>\<Plugin>_artefacts\Release\VST3\` to
   `C:\Program Files\Common Files\VST3\` (plug-in copying is off by default on
   Windows because that folder needs administrator rights).

## Linux

Install the JUCE dependencies listed in
[JUCE's Linux dependency guide](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md),
then build exactly as on macOS. VST3 and Standalone are built.

## Build options

| Option | Default | Meaning |
| --- | --- | --- |
| `OAP_BUILD_PLUGINS` | `ON` | Build the plugins. |
| `OAP_BUILD_TESTS` | `ON` | Build the DSP test runner. |
| `OAP_COPY_PLUGINS` | `ON` on macOS | Copy plugins to the system plug-in folders after building. |
| `OAP_UNIVERSAL_BINARY` | `OFF` | macOS: build for arm64 and x86_64. |
| `OAP_JUCE_PATH` | *(empty)* | Use a local JUCE checkout instead of downloading it. |

## Running the DSP tests

```bash
cmake --build build --target oap_dsp_tests
./build/tests/oap_dsp_tests_artefacts/Release/oap_dsp_tests
```

Add `--render demos` to also write example WAV files (a synthetic guitar played
through both amps) into a `demos/` folder. With CTest: `ctest --test-dir build`.

## Licensing note

JUCE is dual-licensed under the **AGPLv3** and a **commercial JUCE licence**.
If this project is distributed under the AGPLv3 (or a compatible licence), the
free JUCE licence applies. Releasing the plugins under any other terms - for
example closed source - may require a commercial JUCE licence. Check
[JUCE's licensing page](https://juce.com/legal/juce-9-licence/) before choosing
the project licence.
