# AnalogSynth

Virtual analog synthesizer plugin inspired by the Sequential Prophet-6, built with JUCE 7.0.12.

**Plugin formats:** VST3, AU, Standalone

## Build

```bash
cmake -B build && cmake --build build
```

Or with Xcode:

```bash
cmake -B build -G Xcode
open build/AnalogSynth.xcodeproj
```

Build artifacts are placed in `build/AnalogSynth_artefacts/`. The AU and VST3 plugins are automatically copied to `~/Library/Audio/Plug-Ins/` after a successful build.

Standalone app: `build/AnalogSynth_artefacts/Debug/Standalone/AnalogSynth.app`

## Features

- **3 PolyBLEP oscillators** with Saw, Pulse, Triangle, Square waveforms and hard sync
- **Sub oscillator** (Sine/Triangle/Square/Pulse) and white noise generator
- **5 filter types**: Moog Ladder, Diode Ladder, Sequential SVF, TB-303 Acid, Korg MS-20
- **12dB and 24dB slopes** with drive, resonance, and key follow
- **2 ADSR envelopes** (Filter + Amp) with velocity sensitivity
- **LFO** with 5 shapes, tempo sync, and 4 modulation destinations
- **Poly Mod** (Prophet-6 style): Filter Envelope and Osc2 as modulation sources
- **Arpeggiator** with 5 modes, DAW tempo sync, octave range, gate, and latch
- **Effects chain**: Distortion, Chorus/Flanger, Delay (ping-pong, BBD), Reverb (Room/Hall/Plate/Spring)
- **Voice modes**: Poly (6 voices), Mono (legato), Unison (6-voice stack with detune and stereo spread)
- **Analog modeling**: per-voice pitch drift, phase jitter, and component tolerance variation
- **15 factory presets** with user preset save/load
- **Photorealistic UI**: brushed metal faceplate, wood end-cheeks, 3D knobs, LCD oscilloscope

## Documentation

| Document | Audience | Description |
|----------|----------|-------------|
| [User Guide](docs/USER_GUIDE.md) | Musicians, sound designers | Complete feature reference for every synth section |
| [Architecture](docs/ARCHITECTURE.md) | Developers | Signal flow, source layout, and system design |
| [DSP Reference](docs/DSP_REFERENCE.md) | DSP developers | Algorithm details for oscillators, filters, and modulation |
| [Developer Guide](docs/DEVELOPER_GUIDE.md) | Contributors | How to add parameters, effects, filters, and UI panels |
