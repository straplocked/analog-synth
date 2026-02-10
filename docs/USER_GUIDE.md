# AnalogSynth User Guide

A complete reference for every section of the synthesizer.

## UI Layout

```
┌──────────────────────────────────────────────────────────┐
│  ANALOG SYNTH          [< Preset Name >]    Master Vol   │  Header
├──────────────────────────────────────────────────────────┤
│  OSC A      │  OSC B      │  OSC C      │  Waveform     │  Top Row
├─────────────┴──────┬──────┴─────────────┴───────────────┤
│  Filter            │  Mixer    │  Poly Mod               │  Mid Row
├────────────────────┴──────────┴─────────────────────────┤
│  [ ENV ]  [ LFO ]  [ FX ]  [ GLOBAL ]                   │  Tabbed
├──────────────────────────────────────────────────────────┤
│  Pitch │ Mod │ Oct │        MIDI Keyboard                │  Bottom
└──────────────────────────────────────────────────────────┘
```

---

## Oscillators

Three identical oscillators (OSC A, OSC B, OSC C), each with:

| Control | Range | Description |
|---------|-------|-------------|
| Waveform | Saw, Pulse, Triangle, Square | Waveform selection |
| Octave | -2 to +2 | Octave transpose |
| Semi | -12 to +12 st | Semitone transpose |
| Fine | -100 to +100 ct | Fine tuning in cents |
| PW | 5% - 95% | Pulse width (affects Pulse waveform) |
| Level | 0 - 1 | Oscillator output level |

**Hard Sync** (OSC B only): When enabled, Osc1 resets Osc2's phase at each Osc1 cycle, creating harmonically rich timbres. Sweep Osc2's pitch for classic sync sounds.

**Default**: OSC A is a Saw at 80% level. OSC B and C are off (level 0).

## Sub Oscillator

A dedicated low-frequency oscillator that tracks the main pitch.

| Control | Options | Default |
|---------|---------|---------|
| Waveform | Sine, Triangle, Square, Pulse | Triangle |
| Octave | -1 Oct, -2 Oct | -1 Oct |
| Level | 0 - 1 | 0 (off) |

## Noise

White noise generator with a single **Level** control (0-1, default off).

## Mixer

Controls the level balance of all five sound sources before the filter:

- **OSC 1**, **OSC 2**, **OSC 3** — Main oscillator levels
- **SUB** — Sub oscillator level
- **NOISE** — Noise generator level

The mixer applies tanh soft clipping to the sum, preventing harsh digital peaks.

---

## Filter

### Low-Pass Filter

Five classic analog filter models, each with a different character:

| Type | Inspiration | Character |
|------|------------|-----------|
| **Ladder** | Moog | Warm, fat, classic. Self-oscillates at high resonance |
| **Diode** | Roland TB-303 diode ladder | Gritty asymmetric saturation, squelchy |
| **Sequential** | Oberheim/Sequential SVF | Clean, musical resonance across full range |
| **Acid** | TB-303 | Sharp resonance peak, aggressive, bass-heavy |
| **MS20** | Korg MS-20 | Gritty, saturates rather than self-oscillates |

| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| Type | 5 choices | Ladder | Filter model selection |
| Slope | 12dB, 24dB | 24dB | Filter rolloff slope |
| Cutoff | 20 Hz - 20 kHz | 8000 Hz | Filter cutoff frequency |
| Resonance | 0 - 1 | 0 | Resonance / feedback amount |
| Env Amount | -1 to +1 | 0.3 | Filter envelope modulation depth (bipolar) |
| Key Follow | 0 - 1 | 0 | Cutoff tracks keyboard pitch |
| Drive | 1.0 - 5.0 | 1.0 | Pre-filter saturation |

### High-Pass Filter

A transparent 12dB/oct state-variable HPF for removing low-frequency content.

| Control | Range | Default |
|---------|-------|---------|
| HPF Cutoff | 20 Hz - 20 kHz | 20 Hz |

---

## Envelopes

Two independent ADSR envelope generators with velocity sensitivity.

### Filter Envelope

Modulates the low-pass filter cutoff. The **Env Amount** control on the filter section determines how much (and in which direction) this envelope affects the cutoff.

| Control | Range | Default |
|---------|-------|---------|
| Attack | 1 ms - 10 s | 10 ms |
| Decay | 1 ms - 10 s | 300 ms |
| Sustain | 0 - 1 | 0.5 |
| Release | 1 ms - 10 s | 500 ms |
| Velocity | 0 - 1 | 0.5 |

### Amp Envelope

Controls the volume contour of each note.

| Control | Range | Default |
|---------|-------|---------|
| Attack | 1 ms - 10 s | 5 ms |
| Decay | 1 ms - 10 s | 300 ms |
| Sustain | 0 - 1 | 0.8 |
| Release | 1 ms - 10 s | 300 ms |
| Velocity | 0 - 1 | 0.5 |

Both envelopes use exponential curves for natural-sounding dynamics. The **Velocity** control scales envelope depth by MIDI velocity (0 = no velocity response, 1 = full velocity sensitivity).

---

## LFO

A single LFO with five waveform shapes routed to four destinations.

| Control | Range | Default |
|---------|-------|---------|
| Shape | Triangle, Saw, Square, Random (S&H), Sine | Triangle |
| Rate | 0.01 - 30 Hz | 2 Hz |
| Sync | On/Off | Off |
| Sync Div | 1/32, 1/16T, 1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 Bar, 2 Bars, 4 Bars | 1/4 |
| Key Sync | On/Off | Off |

**Destinations** (bipolar, -1 to +1):

| Destination | Effect |
|-------------|--------|
| Osc Freq | Vibrato — pitch modulation of all oscillators |
| Osc Shape | Pulse width modulation (PWM) |
| Filter | Filter cutoff wobble |
| Amp | Tremolo — amplitude modulation |

When **Sync** is enabled, the LFO rate locks to the DAW tempo at the selected division. **Key Sync** resets the LFO phase on each new note.

---

## Poly Mod

Prophet-6 style per-voice modulation with two sources and five destinations each.

### Sources

- **Filter Envelope** — The current filter envelope level
- **OSC 2** — The raw audio output of Oscillator 2 (enables FM, ring mod-like effects)

### Destinations (unipolar, 0 to 1)

| Destination | Effect |
|-------------|--------|
| Freq | Modulates Osc1 frequency (FM synthesis / pitch modulation) |
| Shape | Modulates Osc1 waveform crossfade |
| PW | Modulates Osc1 pulse width |
| LPF | Modulates low-pass filter cutoff |
| HPF | Modulates high-pass filter cutoff |

Poly Mod is applied per-voice, so each voice gets its own modulation based on its own envelope and Osc2 output. This is what enables the metallic, bell-like FM tones and complex filter animation heard in presets like "05 - FM Metallic".

---

## Effects

Four serial effects processed in fixed order. Each effect has an enable toggle.

### 1. Distortion

| Control | Range | Default |
|---------|-------|---------|
| Drive | 1 - 20 | 1 |
| Tone | 0 - 1 | 0.5 |
| Mix | 0 - 1 | 1.0 |

Soft-clipping distortion (tanh saturation) with a tone control that sweeps a low-pass filter from 200 Hz to 10 kHz.

### 2. Chorus / Flanger

| Control | Range | Default |
|---------|-------|---------|
| Rate | 0.1 - 5 Hz | 1 Hz |
| Depth | 0 - 1 | 0.5 |
| Mix | 0 - 1 | 0.5 |
| Flanger | On/Off | Off |

Stereo modulated delay with 90-degree LFO offset between channels. Toggle **Flanger** mode for shorter delay times and comb-filtering effects.

### 3. Delay

| Control | Range | Default |
|---------|-------|---------|
| Time | 1 - 2000 ms | 400 ms |
| Feedback | 0 - 0.95 | 0.4 |
| Mix | 0 - 1 | 0.3 |
| Ping-Pong | On/Off | Off |
| BBD | On/Off | Off |

Stereo delay with Lagrange interpolation. **Ping-Pong** mode bounces repeats between left and right channels. **BBD** mode emulates analog bucket-brigade delays by adding a 3 kHz low-pass filter in the feedback path, progressively darkening each repeat.

### 4. Reverb

| Control | Range | Default |
|---------|-------|---------|
| Type | Room, Hall, Plate, Spring | Room |
| Size | 0 - 1 | 0.5 |
| Damping | 0 - 1 | 0.5 |
| Mix | 0 - 1 | 0.2 |

Four reverb algorithms with independent size and damping controls. Each type maps the Size and Damping knobs to different internal ranges for appropriate behavior.

---

## Arpeggiator

Tempo-synced arpeggiator with automatic DAW tempo detection.

| Control | Range | Default |
|---------|-------|---------|
| Enable | On/Off | Off |
| Mode | Up, Down, Up/Down, Random, As Played | Up |
| Rate | 1/32, 1/16T, 1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 Bar | 1/8 |
| Octaves | 1 - 4 | 1 |
| Gate | 10% - 100% | 75% |
| Latch | On/Off | Off |
| BPM | 30 - 300 | 120 |

- **Up**: Plays held notes from lowest to highest
- **Down**: Highest to lowest
- **Up/Down**: Ascending then descending (ping-pong)
- **Random**: Random note selection from held notes
- **As Played**: Plays notes in the order they were pressed

**Latch** mode holds notes in the sequence after you release the keys. Press new keys to replace the latched pattern.

**BPM** is used in Standalone mode only. When running as a plugin, the arpeggiator automatically syncs to the host DAW's tempo.

---

## Global Controls

### Voice Modes

| Mode | Description |
|------|-------------|
| **Poly** | Standard 6-voice polyphony. Oldest voice is stolen when all 6 are active |
| **Mono** | Single voice with last-note priority. Legato playing (overlapping notes) changes pitch without retriggering envelopes |
| **Unison** | All 6 voices play the same note with configurable detune and stereo spread |

### Unison Controls

| Control | Range | Default |
|---------|-------|---------|
| Detune | 0 - 100 ct | 15 ct |
| Spread | 0 - 1 | 1.0 |

Voices are detuned in pairs (±5ct, ±10ct, ±15ct at default) and panned across the stereo field for a wide, thick sound.

### Other Global Controls

| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| Slop | 0 - 1 | 0.2 | Analog imperfection — adds per-voice pitch drift, phase jitter, and component tolerance variation |
| Glide | On/Off | Off | Enable portamento |
| Glide Time | 1 ms - 2 s | 100 ms | Portamento speed |
| Master Tune | 430 - 450 Hz | 440 Hz | Global tuning reference |
| Pitch Bend Range | 1 - 24 st | 2 st | MIDI pitch bend range |
| Master Volume | -60 to +6 dB | 0 dB | Final output level |

---

## Performance Controls

### Pitch Wheel
Spring-loaded pitch bend wheel. Range is set by the **Pitch Bend Range** parameter (default ±2 semitones). Returns to center on release.

### Mod Wheel
Controls additional modulation. Range 0 to 1.

### Keyboard
On-screen MIDI keyboard with octave shift buttons (or press **Z** / **X** keys). Range: C2 to C7, default octave: C4.

---

## Presets

Factory presets are stored in `~/Library/Application Support/AnalogSynth/Presets/Factory/`.
User presets are stored in `~/Library/Application Support/AnalogSynth/Presets/User/`.

### Browsing Presets

- **Click the preset name** in the header LCD to open a dropdown browser with Factory and User sections
- Use the **<** and **>** buttons to cycle through presets
- The LCD shows **" *"** after the name when you've edited parameters since loading

### Saving & Managing Presets

- **SAVE button**: Overwrites the current user preset, or opens Save As for factory/INIT presets
- **... menu**: Access Save As, Rename, Delete, and Init
  - **Save As**: Save current settings as a new user preset
  - **Rename**: Rename the current user preset (factory presets are read-only)
  - **Delete**: Delete the current user preset (factory presets cannot be deleted)
  - **Init**: Reset all parameters to defaults

### Factory Presets

| # | Name | Description |
|---|------|-------------|
| 01 | Classic Saw Pad | Two detuned saws with filter envelope sweep and reverb |
| 02 | Warm Bass | Saw + sub oscillator, low cutoff, driven filter |
| 03 | Pulse Lead | PWM pulse lead with delay |
| 04 | Dreamy Keys | Triangle + pulse (octave up) with chorus and reverb |
| 05 | FM Metallic | Osc2-to-Osc1 frequency cross-modulation for bell-like tones |
| 06 | Thick Unison | 6-voice unison saw with detune, slop, and distortion |
| 07 | Resonant Sweep | High resonance filter sweep with saw + square |
| 08 | Ethereal Pad | Detuned triangles with LFO filter mod, chorus, and heavy reverb |
| 09 | Pluck | Fast-decay envelope with ping-pong delay |
| 10 | PWM Strings | LFO-modulated pulse width on dual pulse waves with chorus |
| 11 | Acid Squelch | Acid filter, mono mode, glide, high resonance — classic 303 |
| 12 | MS20 Growl | Detuned saws through MS20 filter with drive |
| 13 | Triple Osc Pad | All 3 oscillators through Sequential SVF with chorus and reverb |
| 14 | Deep Sub Sine | Pulse wave + sine sub (-2 oct) with LFO pulse width modulation |
| 15 | Acid Unison | Unison saw + square sub through acid filter with distortion |
| 16 | Finger Bass | Mono saw + sub, fast filter env, Motown-style bass |
| 17 | Reese Bass | Mono detuned saws with distortion and slow LFO movement |
| 18 | Acid Bass 303 | Mono square through acid filter 24dB, high res, glide |
| 19 | Sync Lead | Hard-synced saw with filter sweep and delay |
| 20 | Mono Screamer | Saw + pulse through MS20 filter with LFO vibrato |
| 21 | Portamento Lead | Mono triangle with glide and chorus |
| 22 | Dark Choir | Three oscillators, slow attack, low cutoff, heavy reverb |
| 23 | Glass Shimmer | Triangle + high pulse, long envelopes, chorus + reverb |
| 24 | Analog Strings | Detuned saws through Sequential SVF with ensemble chorus |
| 25 | Electric Piano | Triangle + pulse, medium filter env, velocity-sensitive |
| 26 | Clav | Narrow pulse through diode filter, fast percussive decay |
| 27 | Bell | PolyMod FM (fifth interval) with long release and delay |
| 28 | Wind Noise | Noise-dominant with resonant LFO filter sweep and reverb |
| 29 | Sci-Fi Sweep | PolyMod pitch modulation with long filter attack, cinematic |
| 30 | Arp Sequences | Saw + sub with arp Up 1/16, 2 octaves, ping-pong delay |
| 31 | Ambient Arp | Triangle with arp Random 1/8T, 3 octaves, heavy reverb + chorus |
