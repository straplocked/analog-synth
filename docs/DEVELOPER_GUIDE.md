# Developer Guide

Practical how-to reference for contributing to AnalogSynth.

## Build

```bash
cmake -B build && cmake --build build
```

Xcode: `cmake -B build -G Xcode && open build/AnalogSynth.xcodeproj`

Artifacts in `build/AnalogSynth_artefacts/`. AU and VST3 auto-install to `~/Library/Audio/Plug-Ins/`.

## Code Conventions

- C++17, `juce::` prefix everywhere (no `using namespace juce`)
- `#pragma once` for header guards
- PascalCase classes, camelCase methods/members/constants
- All classes use `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`
- `float` for audio processing, `double` for phase accumulators
- Classes with the leak detector that are default-constructed as members need an explicit `= default` constructor

## JUCE 7.0.12 API Notes

These are specific to JUCE 7 and differ from JUCE 8:

- **Font**: use `juce::Font(float size)`, not `juce::FontOptions` (JUCE 8+)
- **ParameterID**: constructor takes `(stringID, versionHint)` — e.g., `juce::ParameterID { "osc1_level", 1 }`
- **Playhead**: use `getPosition()` returning `Optional<PositionInfo>`, then `getBpm()` etc. The old `getCurrentPosition()` is deprecated
- **AudioProcessorEditor**: `getAudioProcessor()` returns a **pointer** (not reference) — dereference with `*`
- Don't shadow the inherited `processor` member in `AudioProcessorEditor` — use a `getProcessor()` helper instead

## How to Add a New Parameter

1. **Define the ID** in `Source/ParameterIDs.h`:
   ```cpp
   inline constexpr auto myNewParam = "section_myNewParam";
   ```

2. **Add to the layout** in `Source/ParameterLayout.h` inside `createLayout()`:
   ```cpp
   layout.add (std::make_unique<juce::AudioParameterFloat> (
       juce::ParameterID { ParameterIDs::myNewParam, 1 }, "My New Param",
       juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
   ```

3. **Read it on the audio thread** in the appropriate `updateParameters()` method:
   - Voice parameters: `Source/DSP/VoiceManager.h` → `updateParameters(APVTS&)`
   - FX parameters: `Source/FX/FXChain.h` → `updateParameters(APVTS&)`
   - Global parameters: `Source/PluginProcessor.cpp` → `processBlock()`
   ```cpp
   float value = *apvts.getRawParameterValue (ParameterIDs::myNewParam);
   ```

4. **Add UI control** in the relevant panel (e.g., `Source/UI/FilterPanel.h`):
   ```cpp
   RotaryKnob myKnob { "MY PARAM", "Hz" };  // label, unit
   ```

5. **Create the attachment** in the panel's constructor or `attachToAPVTS()`:
   ```cpp
   myKnobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
       apvts, ParameterIDs::myNewParam, myKnob.getSlider());
   ```

6. **Layout the control** in the panel's `resized()` method.

## How to Add a New Effect

1. **Create the effect class** in `Source/FX/`:
   - Follow the pattern of existing effects (e.g., `DistortionEffect.h`)
   - Required methods: `prepare()`, `processBlock()`, `setEnabled()`, `reset()`
   - Add `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`

2. **Add parameters** following the steps above (define IDs, add to layout)

3. **Add to FXChain** (`Source/FX/FXChain.h`):
   - Add the effect as a member
   - Call `prepare()` in `FXChain::prepare()`
   - Call `processBlock()` in the desired position in `FXChain::processBlock()`
   - Read parameters in `FXChain::updateParameters()`

4. **Add UI** in `Source/UI/FXPanel.h` — add a new column with enable button and knobs

## How to Add a New Filter Type

1. **Create the filter class** in `Source/DSP/`:
   - Follow the pattern of existing filters (e.g., `SVFilter.h` for 2-pole, `LadderFilter.h` for 4-pole)
   - Required methods: `prepare()`, `setCutoff()`, `setResonance()`, `setDrive()`, `processSample()`, `reset()`
   - For 4-pole filters: add `setTwoPole(bool)` for 12dB slope support
   - For 2-pole filters: `FilterEngine` will cascade two instances for 24dB mode

2. **Add to FilterEngine** (`Source/DSP/FilterEngine.h`):
   - Add the new type to the `Type` enum
   - Add an instance (or pair for 2-pole) as a member
   - Wire it into `setCutoff()`, `setResonance()`, `setDrive()`, `processSample()`, `reset()`, and `prepare()`

3. **Add to the parameter choice list** in `Source/ParameterLayout.h`:
   - Update the `lpfType` `StringArray` to include the new filter name

4. **Update the UI**: the `FilterPanel` combo box reads from the parameter, so it will automatically show the new option

## UI Architecture

### Component Hierarchy

```
AnalogSynthEditor
  └── MainPanel (1400×800)
        ├── HeaderPanel (preset browser, master volume)
        ├── OscillatorPanel ×3 (top row)
        ├── WaveformDisplay (top row, right)
        ├── FilterPanel (mid row)
        ├── MixerPanel (mid row)
        ├── PolyModPanel (mid row)
        ├── TabbedBottomPanel
        │     ├── EnvelopePanel (2× ADSREditor + knobs)
        │     ├── LFOPanel
        │     ├── FXPanel (4 effect columns)
        │     └── GlobalPanel (voice modes + arp)
        └── Bottom bar
              ├── PitchModWheelPanel
              ├── Octave buttons
              └── MidiKeyboardPanel
```

### Look and Feel

`AnalogSynthLookAndFeel` (`Source/UI/AnalogSynthLookAndFeel.h`) extends `juce::LookAndFeel_V4` with custom rendering for all controls.

**Key color constants** in the `SynthColours` namespace:
- `faceplateLight/Mid/Dark` — brushed metal background
- `sectionOsc` (blue), `sectionFilter` (orange), `sectionEnv` (green), `sectionLfo` (purple), `sectionFx` (red) — per-section accent colors
- `silkscreen/silkscreenDim` — text colors
- `lcdWaveform/lcdText` — oscilloscope green (#33ff99)

**Key rendering methods**:
- `fillBrushedMetal()` — horizontal grain with deterministic pseudo-random brightness
- `drawRecessedPanel()` — inset bevel with inner shadows
- `drawScrewHead()` — Phillips-head screw detail
- `drawSilkscreenLabel()` — embossed text effect
- `drawLED()` — on/off indicator with glow
- `drawRotarySlider()` — photorealistic 3D knob with value ring (31 tick marks)
- `fillWoodGrain()` — multi-frequency sine wave wood texture
- `drawLCDBackground()` — scanlines, grid, glass reflection

**Fonts**: Inter Regular and Inter Bold, embedded via JUCE BinaryData from `Resources/Fonts/`.

### RotaryKnob

The standard knob component (`Source/UI/RotaryKnob.h`) wraps a `juce::Slider` with:
- A label (section name, silkscreen style)
- A value display (formatted with appropriate precision and unit suffix)
- Custom accent color support per knob

**Value formatting**: ≥1000 → "1.5k", ≥100 → integer, ≥10 → 1dp, ≥1 → 2dp, <1 → 3dp.

### WaveformDisplay

Real-time oscilloscope (`Source/UI/WaveformDisplay.h`):
- 2048-sample ring buffer, written by audio thread
- 256-sample display window, read by UI at 30 Hz
- Zero-crossing trigger for stable waveform display
- LCD-style rendering with phosphor glow (three-pass stroke: wide dim → medium → thin bright)

## Common Pitfalls

### Thread Safety
- Always use `getRawParameterValue()` (returns `std::atomic<float>*`) to read parameters on the audio thread — never use `getParameter()->getValue()` which requires the message thread
- The on-screen pitch bend and mod wheel values are `std::atomic<float>` in MainPanel — safe to read from `processBlock()`
- Never allocate memory on the audio thread; all buffers are pre-allocated in `prepare()`

### Oversampling
- The per-voice rendering loop runs at 2× sample rate — buffer sizes passed to `SynthVoice::renderBlock()` are at the native rate, and the voice internally doubles them
- The FIR downsampling filter has state (`firState` array) that must be reset when a voice is restarted
- FX chain runs at native sample rate, not oversampled

### NaN Protection
- `processBlock()` checks for NaN/Inf after voice rendering and again after FX
- If detected: buffer is cleared, FX chain is reset, and a warning is logged
- When adding new DSP: ensure all division operations check for zero denominators, and all feedback paths are bounded

### Filter Coefficient Updates
- `setCutoff()` is called per-sample (for smooth modulation) — it must be fast. Use `FastMath::tan()` instead of `std::tan()`
- `setResonance()` and `setDrive()` are called per-block — they can be slightly more expensive
- Always update all filter instances for resonance/drive (not just the active one) to avoid clicks when switching types
