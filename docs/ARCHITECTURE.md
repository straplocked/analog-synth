# Architecture

System design reference for developers working on AnalogSynth.

## Signal Flow

```
MIDI Input
    │
    ├─ Arp enabled? ──► Arpeggiator ──► VoiceManager
    │                    (per-sample      (noteOn/noteOff)
    │                     timing)
    └─ Arp disabled? ─────────────────► VoiceManager
                                         (noteOn/noteOff)

VoiceManager (6 voices, poly/mono/unison)
    │
    ├── SynthVoice 0 ──┐
    ├── SynthVoice 1 ──┤
    ├── SynthVoice 2 ──┤  Sum to stereo
    ├── SynthVoice 3 ──┤
    ├── SynthVoice 4 ──┤
    └── SynthVoice 5 ──┘
            │
            ▼
    FX Chain (serial)
    ┌─────────────────┐
    │ 1. Distortion    │
    │ 2. Chorus/Flanger│
    │ 3. Delay         │
    │ 4. Reverb        │
    └─────────────────┘
            │
            ▼
    Master Volume (dB → gain)
            │
            ▼
    Audio Output
```

### Per-Voice Signal Chain (2x oversampled)

```
Osc1 ─────┐
Osc2 ─────┤
Osc3 ─────┼──► Mixer (tanh soft clip) ──► LPF ──► HPF ──► Amp Env
Sub  ─────┤                                 ▲       ▲
Noise ────┘                                 │       │
                                      Filter Env    │
                                            │       │
     ◄──── Modulation: LFO + Poly Mod ────►│       │
                                                    │
                              7-tap FIR downsample (2:1)
                                            │
                                            ▼
                                      Voice output (mono)
```

## Source Layout

```
Source/
├── PluginProcessor.h/cpp    Main processBlock, MIDI routing, parameter dispatch
├── PluginEditor.h/cpp       Editor shell: creates MainPanel, owns LookAndFeel
├── ParameterIDs.h           ~142 inline constexpr string IDs for all parameters
├── ParameterLayout.h        createLayout() — ranges, skews, defaults, choices
├── PresetManager.h          XML preset save/load, 15 factory presets
│
├── DSP/
│   ├── VoiceManager.h/cpp   6-voice allocator: poly, mono (legato), unison
│   ├── SynthVoice.h/cpp     Per-voice chain: oscs → mixer → filter → env → output
│   ├── PolyBLEPOscillator.h Band-limited oscillator (Saw/Pulse/Tri/Square + hard sync)
│   ├── SubOscillator.h      Sub osc (Sine/Tri/Square/Pulse, -1/-2 oct)
│   ├── NoiseGenerator.h     White noise (xorshift32)
│   ├── Mixer.h              5-source summing with tanh clipping
│   ├── FilterEngine.h       Router for 5 filter types with slope switching
│   ├── LadderFilter.h       Moog-style TPT 4-pole ladder
│   ├── DiodeLadderFilter.h  Diode ladder with asymmetric clipping
│   ├── SVFilter.h           State-variable filter (Sequential/Oberheim style)
│   ├── AcidFilter.h         TB-303 style 2-pole with algebraic soft clip
│   ├── MS20Filter.h         Korg MS-20 Sallen-Key with OTA saturation
│   ├── StateVariableHPF.h   Transparent 12dB/oct high-pass
│   ├── ADSREnvelope.h       Exponential ADSR with velocity scaling
│   ├── LFO.h                5-shape LFO with tempo sync
│   ├── PolyModRouter.h      Modulation amount structs (PolyModAmounts, LFOAmounts)
│   ├── AnalogDrift.h        Per-voice pitch drift and component tolerance
│   ├── PinkNoise.h          1/f noise (Kellet filter) for jitter/drift
│   ├── Saturation.h         softClip, warmSaturate, tapeStyle functions
│   ├── FastMath.h           Fast exp2/exp/tan approximations
│   └── Arpeggiator.h        5-mode arp with DAW tempo sync
│
├── FX/
│   ├── FXChain.h/cpp        Serial FX router with NaN safety
│   ├── DistortionEffect.h   tanh drive + tone LP + dry/wet
│   ├── ChorusEffect.h       Dual stereo delay lines, sine LFO, flanger mode
│   ├── DelayEffect.h        Stereo delay, ping-pong, BBD emulation
│   └── ReverbEffect.h       JUCE dsp::Reverb wrapper (Room/Hall/Plate/Spring)
│
├── UI/
│   ├── MainPanel.h/cpp      Top-level layout (1400×800), focus management
│   ├── HeaderPanel.h        Preset browser, master volume, logo
│   ├── OscillatorPanel.h    Waveform selector + tuning knobs (×3)
│   ├── FilterPanel.h        Type/slope selectors + filter knobs
│   ├── MixerPanel.h         5 level knobs + sub waveform/octave
│   ├── PolyModPanel.h       2×5 modulation amount knobs (housed in tabbed panel)
│   ├── TabbedBottomPanel.h  Tab container for ENV/LFO/POLY MOD/FX/GLOBAL
│   ├── EnvelopePanel.h      Dual ADSR sections with visual editors
│   ├── ADSREditor.h         Visual envelope curve with LCD styling
│   ├── LFOPanel.h           Shape/rate/sync + destination amount knobs
│   ├── FXPanel.h            4-column FX controls
│   ├── GlobalPanel.h        Voice mode + arpeggiator controls
│   ├── RotaryKnob.h         Standard knob component with label + value display
│   ├── WaveformDisplay.h    Real-time oscilloscope (ring buffer, 30Hz refresh)
│   ├── PitchModWheelPanel.h Pitch bend + mod wheel sliders
│   ├── MidiKeyboardPanel.h  On-screen keyboard with octave buttons
│   └── AnalogSynthLookAndFeel.h/cpp  Custom theme: metal, wood, 3D knobs, LEDs, imperfections
│
└── Utility/
    ├── DebugLog.h           Optional file logging to Desktop
    ├── SmoothParam.h        One-pole parameter smoother
    └── MathHelpers.h        Shared math utilities
```

## Parameter System

Parameters flow through four layers:

```
ParameterIDs.h          String constants ("osc1_waveform", "lpf_cutoff", etc.)
        │
ParameterLayout.h       createLayout() — defines type, range, skew, default
        │
APVTS                   AudioProcessorValueTreeState — thread-safe storage
        │
        ├──► UI: SliderAttachment / ButtonAttachment / ComboBoxAttachment
        │        (bidirectional binding to RotaryKnob, toggle buttons, combo boxes)
        │
        └──► Audio thread: getRawParameterValue() returns std::atomic<float>*
                  │
                  ▼
              VoiceManager::updateParameters(apvts)  — reads all voice params per block
              FXChain::updateParameters(apvts)        — reads all FX params per block
```

Common parameter range types defined in `ParameterLayout.h`:
- `freqRange`: 20–20kHz, skew 0.25 (logarithmic feel)
- `timeRange`: 1ms–10s, skew 0.3
- `pctRange`: 0–1, linear
- `biRange`: -1 to +1, linear

## processBlock Pipeline

The processor's `processBlock()` method runs these steps each audio buffer:

1. **Clear buffer**
2. **Merge on-screen keyboard** MIDI into the buffer
3. **Read DAW tempo** (fallback to standalone BPM parameter)
4. **Update arpeggiator** parameters from APVTS
5. **Update voice parameters** — `voiceManager.updateParameters(apvts)`
6. **Route MIDI**:
   - Arp enabled: notes go to Arpeggiator, which emits timed events to VoiceManager
   - Arp disabled: notes go directly to VoiceManager
   - Pitch bend and mod wheel always go to VoiceManager
7. **Override wheels** if on-screen pitch/mod wheels have non-default values
8. **Render voices** — `voiceManager.renderBlock()` sums all active voices to stereo
9. **NaN/Inf check** — clear buffer if bad values detected
10. **Process FX** — `fxChain.processBlock()` (Distortion → Chorus → Delay → Reverb)
11. **NaN/Inf check** — second safety check after FX
12. **Apply master volume** (dB to linear gain)
13. **Push to waveform display** (left channel → ring buffer)

## Voice Management

**VoiceManager** allocates 6 `SynthVoice` instances with three operating modes:

### Poly Mode
- Standard polyphonic allocation
- Voice stealing: oldest active voice is stolen when all 6 are busy
- Voice age tracked via incrementing counter

### Mono Mode
- Single voice with a held-notes stack (last-note priority)
- **Legato**: when a new note is pressed while another is held, the voice changes pitch via `setNote()` without retriggering envelopes
- When all keys are released, the note below (if any) is restored from the stack

### Unison Mode
- All 6 voices play the same MIDI note
- Voices are detuned in pairs: voice 0/1 at ±5ct, voice 2/3 at ±10ct, voice 4/5 at ±15ct (scaled by detune parameter)
- Each voice gets a random start phase for natural stacking
- Stereo spread pans voices across the field

## Modulation Routing

### LFO (global per-voice)
Each voice has its own LFO instance. Destinations are applied per-sample in the voice rendering loop:
- **Osc Freq**: adds to all oscillator frequencies (vibrato)
- **Osc Shape**: modulates pulse width
- **Filter**: adds to LPF cutoff
- **Amp**: scales output amplitude (tremolo)

### Poly Mod (per-voice, Prophet-6 style)
Applied per-sample inside each voice. Two modulation sources:
- **Filter Envelope** output (0 to 1)
- **Osc2 audio output** (raw waveform, bipolar)

Each source routes to 5 destinations: Osc1 Freq, Osc1 Shape, Osc1 PW, LPF Cutoff, HPF Cutoff.

### Analog Drift
Each voice has an `AnalogDrift` instance providing:
- **Pitch drift**: 3 sine oscillators at irrational frequency ratios (golden-ratio based), combined for slow VCO wander
- **Component tolerances**: per-voice offsets for filter cutoff (±5%), envelope time (±5%), resonance (±4%), drive (±3%), LFO rate (±5%)
- **Phase jitter**: PinkNoise generators on each oscillator
- **DC offset**: slight bias for even harmonic content

Amount controlled by the global **Slop** parameter (default 0.2).

## Thread Safety

- **APVTS parameters**: `getRawParameterValue()` returns `std::atomic<float>*`, safe to read on audio thread without locks
- **Waveform display**: ring buffer with atomic write index; audio thread writes, UI timer reads
- **On-screen wheels**: pitch bend and mod wheel values stored as `std::atomic<float>` in MainPanel, read by audio thread
- **Preset load**: `apvts.replaceState()` is thread-safe (JUCE handles locking internally)

## Preset System

- **Format**: XML files (JUCE `ValueTree` serialization)
- **Location**: `~/Library/Application Support/AnalogSynth/Presets/`
- **Factory presets**: 15 presets auto-generated on first run; versioned via `_factory_v{N}.marker` file
- **Version upgrades**: old factory presets are deleted and rebuilt when the version constant (`kFactoryPresetVersion`) increments
- **User presets**: saved alongside factory presets as `{name}.xml`
