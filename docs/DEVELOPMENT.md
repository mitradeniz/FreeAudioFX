# Developer guide

## Repository layout

```text
FreeAudioFX/
├── CMakeLists.txt              # top level: JUCE, shared code, plugins, tests
├── cmake/OapPlugin.cmake       # oap_add_plugin() - common plugin settings
├── shared/
│   ├── dsp/                    # reusable DSP blocks (header-only, mostly plain C++17)
│   │   ├── Filters.h           # TPT one-pole, RBJ biquads, envelope follower
│   │   ├── TubeStage.h         # triode curve, preamp gain stage, power amp with sag
│   │   ├── ToneStack.h         # passive TMB tone stack (Yeh/Smith model)
│   │   ├── NoiseGate.h         # hysteresis/hold noise gate
│   │   ├── CabinetModel.h      # filter-based cabinet voicings
│   │   └── CabinetSection.h    # voicings + user IR convolution, click-free switching
│   ├── plugin/AmpProcessorBase # buses, state, IR loading, bypass latency
│   └── ui/                     # look-and-feel, knobs, switches, cab panel, scalable editor
├── plugins/
│   ├── DriveAmp/Source/        # DriveAmpEngine (DSP), parameters, processor, editor
│   └── CleanAmp/Source/        # CleanAmpEngine (DSP), parameters, processor, editor
├── tests/                      # console test runner for the DSP
└── docs/
```

Each plugin keeps its complete signal path in an **engine** class
(`DriveAmpEngine`, `CleanAmpEngine`) that knows nothing about the plugin
wrapper. The processor only reads parameters and calls `engine.process()`, and
the tests drive the engines directly.

## Adding a new plugin

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
3. Never change a released plugin's `PLUGIN_CODE` or parameter IDs - hosts use
   them to find the plugin and restore sessions/automation.

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

## Tests

`tests/DspTests.cpp` checks, among other things:

- biquad designs and the tone stack model (the passive network must never
  exceed 0 dB, and every control must move the right frequencies),
- stability with randomised parameter automation at 44.1-192 kHz and block
  sizes from 1 to 2048 samples,
- decay to true silence (no denormal noise or DC),
- noise-gate behaviour, output levels, distortion amounts, aliasing,
- IR loading, level matching and click-free cabinet switching,
- CPU cost.

Run it after every DSP change. For host-level testing,
[pluginval](https://github.com/Tracktion/pluginval) is recommended:

```bash
pluginval --strictness-level 10 --validate "path/to/OAP Drive Amp.vst3"
```

## Tuning the sound

The voicing constants (stage gains, biases, filter corners, knob ranges) live
at the top of each engine's `.cpp` file and in its `prepare()` method. Change
them, rebuild the Standalone app, play, repeat - then re-run the tests so the
level and aliasing checks still pass.
