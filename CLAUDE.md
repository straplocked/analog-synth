# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Virtual analog synthesizer plugin modeled after the Sequential Prophet-6, built with JUCE 7.0.12. Outputs VST3, AU, and Standalone formats.

## Build

```bash
cmake -B build && cmake --build build
```

Xcode: `cmake -B build -G Xcode && open build/AnalogSynth.xcodeproj`

Artifacts land in `build/AnalogSynth_artefacts/`. AU/VST3 auto-install to `~/Library/Audio/Plug-Ins/` via `COPY_PLUGIN_AFTER_BUILD`.

## Architecture

### Signal Flow

```
MIDI → Arpeggiator (if enabled) → VoiceManager (6 voices)
  Per-voice (2x oversampled):
    3 PolyBLEP Oscillators + Sub + Noise → Mixer (tanh clip)
      → LPF (5 types: Ladder/Diode/Sequential/Acid/MS20) → HPF
      → Filter Envelope + Amp Envelope
      → Modulation (LFO + Poly Mod)
    Downsample (7-tap FIR)
  → Voice sum (stereo)
→ FX Chain: Distortion → Chorus → Delay → Reverb
→ Master Volume → Output
```

### Source Layout

- `Source/PluginProcessor.cpp` — Main processBlock: MIDI routing, parameter reads from APVTS, voice rendering, FX, master gain, NaN checks
- `Source/PluginEditor.cpp` — Creates MainPanel, owns LookAndFeel
- `Source/ParameterIDs.h` — ~142 `inline constexpr` string IDs for every parameter
- `Source/ParameterLayout.h` — `createParameterLayout()` with custom skewed ranges
- `Source/PresetManager.h` — XML preset save/load, 15 factory presets in `~/Library/Application Support/AnalogSynth/Presets/`
- `Source/DSP/` — Voice engine: `VoiceManager`, `SynthVoice`, `PolyBLEPOscillator`, `FilterEngine`, `LadderFilter`, `ADSREnvelope`, `LFO`, `AnalogDrift`
- `Source/FX/` — `FXChain` (serial), `DistortionEffect`, `ChorusEffect`, `DelayEffect`, `ReverbEffect`
- `Source/UI/` — `MainPanel`, `RotaryKnob`, panel classes, `AnalogSynthLookAndFeel` (photorealistic theme with imperfections), `WaveformDisplay`
- `Source/Utility/` — `DebugLog`, `SmoothParam`, `MathHelpers`

### Parameter Flow

UI components attach to APVTS via `SliderAttachment`/`ButtonAttachment`/`ComboBoxAttachment`. In `processBlock()`, the processor reads raw parameter values and pushes them into VoiceManager and FXChain via `updateParameters()` methods.

### Voice Modes

- **Poly**: 6 voices, oldest-voice stealing
- **Mono**: single voice, last-note priority, legato (no retrigger on overlap)
- **Unison**: all 6 voices on one note, paired detune (±5/10/15ct), stereo spread

### Modulation

- **LFO** (global): routes to osc freq, pulse width, filter cutoff, amplitude; tempo-syncable
- **Poly Mod** (per-voice, Prophet-6 style): Filter Envelope and Osc2 audio as sources → Osc1 freq/shape/PW, filter cutoffs

## Conventions

- C++17, `juce::` prefix everywhere (no `using namespace juce`)
- `#pragma once` for header guards
- PascalCase classes, camelCase methods/members/constants
- All classes use `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`
- `float` for audio processing, `double` for phase accumulators
- JUCE 7.0.12 API: use `juce::Font(float)` not `FontOptions`, `ParameterID(stringID, versionHint)`, `getPosition()` not `getCurrentPosition()` for playhead
