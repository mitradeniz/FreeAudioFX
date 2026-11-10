# Developer guide

## Repository layout

```text
FreeAudioFX/
├── CMakeLists.txt              # top level: JUCE, shared code, plugins, tests
├── cmake/
│   ├── OapJuce.cmake           # compiles JUCE once (oap_juce) for every plugin
│   └── OapPlugin.cmake         # oap_add_plugin() - common plugin settings
├── shared/
│   ├── dsp/                    # amp DSP blocks (header-only, mostly plain C++17)
│   │   ├── Filters.h           # TPT one-pole, RBJ biquads, envelope follower
│   │   ├── TubeStage.h         # triode curve, preamp gain stage, power amp with sag
│   │   ├── ToneStack.h         # passive TMB tone stack (Yeh/Smith model)
│   │   ├── NoiseGate.h         # hysteresis/hold noise gate
│   │   ├── CabinetModel.h      # filter-based cabinet voicings
│   │   └── CabinetSection.h    # voicings + user IR convolution, click-free switching
│   ├── fx/                     # effects library (plain C++17, no JUCE): every pedal
│   │   ├── Effect.h            # ParamSpec, EffectInfo, Effect base class, registry API
│   │   ├── FxDsp.h             # delay line, LFO, SVF, all-passes, oversampler, mix laws
│   │   ├── FxEngines.h         # FDN reverb, spring reverb, pitch shifter
│   │   ├── *Effects.cpp        # the 20 effects, grouped by family
│   │   └── Registry.cpp        # display order, lookup, factory
│   ├── amps/                   # effect amps
│   │   ├── AmpModel.h          # what an effect amp is: voicing, effects, FX knobs, colours
│   │   ├── AmpModels.cpp       # the effect amps
│   │   ├── EffectAmpEngine.*   # the signal path built from a model
│   │   └── EffectAmp{Processor,Editor}.*  # generic plugin wrapper and faceplate
│   ├── art/                    # one SVG artwork per amp (generated, embedded into the plugins)
│   ├── plugin/                 # AmpProcessorBase, PedalProcessor, PedalboardProcessor
│   └── ui/                     # look-and-feel, knobs, switches, cab panel, pedal and board editors
├── plugins/
│   ├── DriveAmp/Source/        # DriveAmpEngine (DSP), parameters, processor, editor
│   ├── CleanAmp/Source/        # CleanAmpEngine (DSP), parameters, processor, editor
│   ├── effectamps/             # 10 targets from one source file
│   ├── pedals/                 # 20 targets from one source file
│   └── Pedalboard/
├── tests/                      # console test runner (amps, effects, effect amps)
├── tools/
│   ├── Snapshots.cpp           # renders editors to PNG for the docs
│   └── art/make_amp_art.py     # draws the amp artwork (shared/art/*.svg)
└── docs/
```

The DSP always lives in classes that know nothing about the plugin wrapper:
the amp **engines** (`DriveAmpEngine`, `CleanAmpEngine`, `EffectAmpEngine`) and
the **effects** in `shared/fx`. Processors only read parameters and call
`process()`, and the tests drive the DSP directly.

The pedals, the effect amps and the Pedalboard are **data-driven**: their
parameters, editors and saved state are generated from an `EffectInfo` or an
`AmpModel`, so most new plugins need no UI or wrapper code at all.

## Adding a pedal

1. Write the effect in the matching `shared/fx/*Effects.cpp` file: a class
   derived from `fx::Effect` with a static `describe()` returning its
   `EffectInfo` (id, name, category, one-line description, enclosure colour,
   advanced flag, and at most six `ParamSpec`s), plus `prepare()`, `reset()`
   and `process()` for one or two channels.
2. Register it in the file's `add…Effects()` function and add its id to the
   **end** of `displayOrder` in `Registry.cpp` (the Pedalboard's *Pedal*
   parameters store positions in this list, so inserting in the middle would
   change pedals in saved sessions).
3. Add one line to `plugins/pedals/CMakeLists.txt`:

   ```cmake
   oap_add_pedal(MyPedal myeffect "OAP My Effect" Myfx "What it does" Fx Modulation)
   ```

That's it: the pedal plugin, its stompbox editor and its Pedalboard slot all
come from the description. The effect is also available to the effect amps.
Run the tests: `FxTests.cpp` automatically stress-tests, level-checks and
benchmarks every registered effect.

## Adding an effect amp

1. Add a function to `shared/amps/AmpModels.cpp` that fills an `AmpModel`, and
   list it in `allAmpModels()`:
   - `voicing`: input high-pass, bright and pre-mid EQ, one to four
     `TubeStage` settings (gain, bias, coupling, bandwidth), Gain knob range,
     tone stack (`bassman`, `british`, `blackface`), post EQ, power amp sag,
     4x or 8x oversampling, an optional gate and the default cabinet;
   - `effects`: effects from the library, before or after the amp, with the
     parameters that stay fixed;
   - `fxControls`: two to five extra knobs (`ParamSpec`s), and
     `applyControls`, which maps them onto the effects' parameters every block;
   - `look`: background, faceplate and accent colours.
2. Give it artwork (see [Amp artwork](#amp-artwork) below): a scene in
   `tools/art/make_amp_art.py` named after the amp's id, and an entry in the
   style table in `shared/amps/EffectAmpEditor.cpp` (ornament and faceplate
   opacity).
3. Add one line to `plugins/effectamps/CMakeLists.txt`:

   ```cmake
   oap_add_effect_amp(MyAmp myamp "OAP My Amp" Myam "What it sounds like")
   ```

4. Run the tests. `EffectAmpTests.cpp` checks that every effect and parameter
   name in the model exists, that every FX knob changes the sound, stability,
   decay to silence and loudness. Adjust `voicing.outputTrim` until the
   default level lands in the collection's window (clean amps around -19 dB
   RMS, driven ones around -15 dB RMS), and use `gainCompensation` /
   `masterCompensation` if the top of the Gain or Master range gets too loud.

## Amp artwork

Every amp head is covered by an illustration: a 1200 x 400 SVG in
`shared/art/<id>.svg`, embedded into the plugins with `juce_add_binary_data`
and drawn behind a see-through faceplate. The SVGs are generated by
`tools/art/make_amp_art.py` (plain Python, no dependencies), which has one
function per scene built from small helpers (stars, glows, mountains, palms,
flames, bolts, blossoms and so on):

```bash
python3 tools/art/make_amp_art.py   # rewrites shared/art/*.svg
```

The scene is scaled to cover the whole amp, anchored at the top. Put the main
motif around x = 705 near the top (between the logo and the brand on every
amp) and keep the logo area and the brand area calm; the top and bottom bands
show the art fully, the faceplate shows it faintly.

The theme's **ornament** frames each control so it looks like part of the
artwork: `halo`, `neon`, `pixel`, `thorns`, `ornate`, `stitch`, `petals`,
`star`, `rays`, `bolts`, `pentagram`, `orbit`, `sparkles` or `rose`
(`AmpLookAndFeel::paintOrnament`). With an ornament the knobs are drawn a
little smaller so neighbouring rings never overlap. Render every editor with
`oap_snapshots` to check the result.

## Adding a completely new plugin

1. Create `plugins/MyEffect/` with `Source/` and a `CMakeLists.txt`:

   ```cmake
   oap_add_plugin(MyEffect
       PRODUCT_NAME    "OAP My Effect"
       PLUGIN_CODE     Myfx          # 4 characters, exactly one upper-case letter, unique
       DESCRIPTION     "What it does"
       VST3_CATEGORIES Fx
       SOURCES Source/PluginProcessor.cpp Source/PluginEditor.cpp)
   ```

2. Add `add_subdirectory(plugins/MyEffect)` to the top-level `CMakeLists.txt`.

> [!IMPORTANT]
> Never change a released plugin's `PLUGIN_CODE`, effect or model **id**, or
> parameter IDs: hosts use them to find the plugin and to restore sessions and
> automation.

## Real-time rules for DSP code

- No memory allocation, locks, file or console I/O in `process()`. Allocate in
  `prepare()`.
- Smooth every parameter that changes gain or filter coefficients.
- Non-linear stages run oversampled; linear filters run at the base rate where
  possible.
- Handle any block size, including blocks larger than the size given to
  `prepare()` (some hosts do this).
- Guard against denormals (`juce::ScopedNoDenormals` in `processBlock`, and
  flush decaying states to zero).
- Report latency and tail length to the host.
- Portability: the code is built with GCC, Clang and MSVC on three platforms.
  Don't pass explicit template arguments to `juce::jmax`/`jmin`/`jlimit`
  (some instantiations don't exist on ARM); use `std::max<T>` instead.

## Tests

`tests/` builds one console runner, `oap_dsp_tests`:

- `DspTests.cpp`: biquads, the tone stack model, cabinets and IRs, the Drive
  and Clean amps (stability at 44.1-192 kHz and block sizes 1-2048, silence,
  gate, levels, distortion amounts, aliasing, click-free cabinet switching, CPU),
- `FxTests.cpp`: the effect building blocks and every effect (parameter specs,
  stress with random automation, decay to silence, levels, CPU),
- `EffectAmpTests.cpp`: every effect amp (model validity, stress, silence,
  calibrated levels, FX knobs, gates).

Run it after every DSP change. For host-level testing,
[pluginval](https://github.com/Tracktion/pluginval) is recommended:

```bash
pluginval --strictness-level 10 --validate "path/to/OAP Drive Amp.vst3"
```

## Tuning the sound

The voicing constants live at the top of each engine's `.cpp` file and in its
`prepare()` method (Drive and Clean amps), in the model entries in
`AmpModels.cpp` (effect amps), and in each effect's class in `shared/fx`.
Change them, rebuild the Standalone app or the plugin, play, repeat, then
re-run the tests so the level and stability checks still pass.

## Commits

The project is public, so commit messages are in English: a short imperative
subject ("Add tape echo pedal", "Fix denormals in the spring reverb"), a blank
line, then what changed and why.
