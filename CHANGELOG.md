# Changelog

All notable changes to AnalogSynth will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2025-02-11

### Added

- 3 PolyBLEP oscillators (Saw, Pulse, Triangle, Square) with hard sync
- Sub oscillator (Sine, Triangle, Square, Pulse) and white noise generator
- 5 analog-modeled filter types: Moog Ladder, Diode Ladder, Sequential SVF, TB-303 Acid, Korg MS-20
- Switchable 12dB/24dB filter slopes with drive, resonance, and key follow
- 2 ADSR envelopes (Filter + Amp) with velocity sensitivity
- LFO with 5 shapes, tempo sync, and 4 modulation destinations
- Prophet-6 style Poly Mod (Filter Envelope and Osc2 as sources)
- Arpeggiator with Up, Down, Up/Down, Random, and Order modes
- 6-voice polyphony with Poly, Mono (legato), and Unison voice modes
- Reorderable effects chain: Distortion (7 types), Compressor, Parametric EQ, Chorus/Flanger, Delay (tempo sync, ping-pong, BBD), Reverb (Room/Hall/Plate/Spring)
- Drag-and-drop eurorack-style FX module reordering
- Per-voice analog drift, phase jitter, and component tolerance modeling
- 2x oversampled voice rendering with 7-tap FIR decimation
- 15 factory presets with user preset save/load (XML format)
- Photorealistic UI with brushed metal, wood end-cheeks, 3D knobs, and LCD oscilloscope
- VST3, AU, and Standalone plugin formats
- 86 unit tests covering all DSP and utility classes
