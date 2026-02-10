# DSP Reference

Detailed algorithm documentation for the AnalogSynth sound engine.

## Oscillators

### PolyBLEP Algorithm

All main oscillators (`Source/DSP/PolyBLEPOscillator.h`) use the Polynomial Band-Limited Step (PolyBLEP) technique to suppress aliasing at waveform discontinuities. A parabolic correction is applied within one sample period of each transition:

```
polyBLEP(t):
    if t < dt:   return -(t/dt)² + 2(t/dt) - 1
    if t > 1-dt: return  (t-1+dt)²/dt² - 2(t-1)/dt - 1  (leading correction)
    else:        return 0
```

where `dt = frequency / sampleRate` (one sample's worth of phase).

### Waveforms

| Waveform | Generation |
|----------|-----------|
| **Saw** | Naive ramp (2×phase - 1) with PolyBLEP correction at the wrap discontinuity |
| **Pulse** | Difference of two saws offset by pulse width, with PolyBLEP on both edges. Uses averaged PW for smooth modulation |
| **Square** | Special case of Pulse at 50% duty cycle |
| **Triangle** | Leaky integrator of a PolyBLEP square wave, with DC correction factor `4×freq/sampleRate` |

### Hard Sync

Osc1 acts as the master, Osc2 as the slave (when sync is enabled on OSC B):

1. Each sample, check if Osc1's phase wrapped (`didPhaseWrap()`)
2. If so, get the fractional sample position where the wrap occurred (`getWrapFraction()`)
3. Force Osc2's phase reset at that fractional position (`syncResetAt()`)
4. Apply quadratic BLEP correction at the sync discontinuity to suppress aliasing

The fractional-sample accuracy ensures clean sync sweeps without audible stepping.

### Sub Oscillator

`Source/DSP/SubOscillator.h` — dedicated oscillator tracking the main pitch at -1 or -2 octaves.

- **Sine**: direct `sin(2π × phase)` — no aliasing possible
- **Triangle**: geometric computation from phase
- **Square/Pulse**: PolyBLEP anti-aliased, same algorithm as main oscillators

---

## Filters

All filters use **TPT (Topology-Preserving Transform)** for numerical stability and accurate frequency response. The basic TPT coefficient is:

```
g = tan(π × cutoff / sampleRate)
G = g / (1 + g)
```

### FilterEngine Router

`Source/DSP/FilterEngine.h` manages all five filter instances. Key design decisions:

- **Slope switching**: 4-pole filters (Ladder, Diode) support 12dB mode by tapping an intermediate stage (`setTwoPole(true)`). 2-pole filters (SVF, Acid, MS20) achieve 24dB by cascading two filter instances.
- **Performance**: `setCutoff()` only updates the currently active filter (called per-sample for smooth modulation). `setResonance()` and `setDrive()` update all filters (called per-block) to ensure click-free switching between types.

### Ladder Filter (Moog-style)

`Source/DSP/LadderFilter.h` — Välimäki/Huovilainen TPT ladder model.

- 4 cascaded one-pole stages with nonlinear feedback
- Feedback path: `tanh(input - resonance × stage[3])` (self-oscillation near resonance=4)
- Each stage: trapezoidal integration `v = G × (tanh(input) - state); output = v + state; state += 2v`
- Resonance compensation: attenuates input at high Q to model bass loss (`1.0 / (1.0 + resonance × 0.15)`)
- Drive: `tanh(input × drive)` pre-saturation
- 12dB mode: taps output from `stage[1]` instead of `stage[3]`

### Diode Ladder Filter

`Source/DSP/DiodeLadderFilter.h` — Stilson/Smith diode ladder topology.

- Asymmetric diode clipping: positive side softer (0.8× tanh), negative side harder (1.2× tanh)
- Weighted feedback sum: `0.5×stage[3] + 0.3×stage[2] + 0.15×stage[1] + 0.05×stage[0]`
- Higher resonance range (0–8) due to weighted tapping
- DC blocker (5 Hz high-pass) to remove offset from asymmetric clipping
- Resonance compensation: `1.15 / (1.0 + resonance × 0.08)`

### Sequential SVF (Oberheim-style)

`Source/DSP/SVFilter.h` — TPT state-variable filter, low-pass output.

- 2-pole with Q range 0.5–12 (R = 1/(2Q))
- Clean, musical resonance; unity passband gain
- Baseline attenuation of 0.33× to compensate for filter gain
- Drive: `tanh(input × drive)` at input

### Acid Filter (TB-303-style)

`Source/DSP/AcidFilter.h` — 2-pole diode ladder with distinctive resonance.

- Algebraic soft clip: `x / (1 + |x|)` (sharper than tanh)
- 1.5× makeup gain to compensate for heavy soft-clip attenuation
- Feedback uses `acidClip()` on `stage[1]`; internal stages use `tanh()` to avoid triple compression
- DC blocker (5 Hz high-pass)

### MS20 Filter (Korg MS-20-style)

`Source/DSP/MS20Filter.h` — Sallen-Key topology with OTA/diode saturation.

- Piecewise asymmetric saturation function:
  - `x > 1`: soft positive limit with `tanh`
  - `x < -1`: harder negative clip with `log1p` (diode characteristic)
  - `|x| ≤ 1`: linear passthrough
- Resonance range 0–2; saturates aggressively rather than self-oscillating cleanly
- Input pre-clipped with `tanh` to tame extreme feedback

### State-Variable HPF

`Source/DSP/StateVariableHPF.h` — TPT state-variable filter, high-pass output only.

- 2-pole (12dB/oct), fixed Q ≈ 0.707 (Butterworth response)
- No resonance or drive controls

---

## Envelopes

### ADSR Envelope

`Source/DSP/ADSREnvelope.h` — exponential envelope with four stages.

**Coefficient formula:**
```
coefficient = 1.0 - exp(-1.0 / (sampleRate × timeInSeconds))
```

This produces an exponential curve reaching ~63% of the target in the specified time.

**Stage behavior:**
- **Attack**: exponential rise toward overshoot target (1.3) for snappy response; transitions to Decay when level ≥ 1.0
- **Decay**: exponential fall toward sustain level
- **Sustain**: holds at sustain level
- **Release**: exponential fall toward 0; transitions to Idle when level < 0.0001

**Velocity**: output is `level × velocity`, where velocity is the value passed to `noteOn()` (0.0–1.0).

Minimum time for A/D/R is clamped to 1 ms to prevent discontinuities.

---

## LFO

`Source/DSP/LFO.h` — low-frequency oscillator with 5 shapes.

| Shape | Algorithm |
|-------|-----------|
| Triangle | `4 × |phase - 0.5| - 1` (bipolar) |
| Saw | `1 - 2 × phase` (descending ramp) |
| Square | `phase < 0.5 ? 1 : -1` |
| Random | Sample-and-hold; new random value via `xorshift32` at each cycle start |
| Sine | `sin(2π × phase)` |

**Tempo sync**: when enabled, the LFO rate is calculated from the host BPM and the selected note division. The division-to-beat mapping follows standard musical notation (1/4 = 1 beat, 1/8 = 0.5 beats, triplets scaled by 2/3).

**Key sync**: when enabled, phase resets to 0 on each `noteOn()`.

---

## Analog Modeling

### AnalogDrift

`Source/DSP/AnalogDrift.h` — per-voice VCO drift and component tolerance simulation.

**Pitch drift**: three sine oscillators at frequencies 0.05 Hz, 0.11 Hz, and 0.077 Hz (irrational ratios prevent repeating patterns). Their weighted sum produces slow, organic pitch wander.

**Per-voice tolerances** (fixed offsets based on voice index, using golden ratio and prime number seeds):

| Parameter | Variation |
|-----------|-----------|
| Filter cutoff | ±5% |
| Envelope time | ±5% |
| Resonance | ±4% |
| Drive | ±3% |
| LFO rate | ±5% |
| DC offset | ±0.5% |

### PinkNoise

`Source/DSP/PinkNoise.h` — 1/f noise using Paul Kellet's 7-stage IIR pinking filter.

Used for oscillator phase jitter (one PinkNoise per oscillator per voice) and noise floor simulation. Seeded uniquely per voice for non-correlated drift.

---

## Oversampling

`Source/DSP/SynthVoice.h` — 2× oversampling for the per-voice signal path.

- Oscillators and filters render at `2 × sampleRate` (typically 88.2 kHz)
- After per-voice processing, a **7-tap half-band FIR filter** decimates back to the original rate
- FIR coefficients: `{-0.045, 0, 0.295, 0.5, 0.295, 0, -0.045}`
- This suppresses aliasing from oscillator harmonics and filter nonlinearities above Nyquist

The FX chain runs at the native sample rate (not oversampled).

---

## Saturation Functions

`Source/DSP/Saturation.h` — three saturation algorithms:

| Function | Formula | Character |
|----------|---------|-----------|
| `softClip(x)` | `tanh(x)` | Standard symmetric soft clipping |
| `warmSaturate(x, drive)` | `tanh(x×drive) + 0.05×driven×|driven|` | Asymmetric — adds even harmonics for warmth |
| `tapeStyle(x, drive)` | Cubic below threshold, hard limit above | Tape-style gentle compression |

---

## FastMath

`Source/DSP/FastMath.h` — fast approximations for per-sample filter coefficient updates.

| Function | Method | Accuracy |
|----------|--------|----------|
| `exp2(x)` | IEEE 754 exponent manipulation + degree-4 minimax polynomial | ~20-bit (~1e-6 max error) |
| `exp(x)` | Implemented as `exp2(x × log2(e))` | Same as exp2 |
| `tan(x)` | Padé [2/1] rational: `x(15 - x²) / (15 - 6x²)` | <0.3% error for \|x\| < 1.0 |

These replace `std::exp` and `std::tan` in the inner rendering loop where filter coefficients are recalculated every sample (for smooth LFO/envelope modulation of cutoff).

---

## Arpeggiator

`Source/DSP/Arpeggiator.h` — per-sample timing engine.

**Timing**: the arpeggiator processes one sample at a time via `processSample()`, maintaining an internal sample counter. When the counter reaches the step duration (calculated from BPM and sync division), it emits a note-on event. After `gate × stepDuration` samples, it emits a note-off.

**Note ordering by mode**:
- **Up**: sort ascending
- **Down**: sort descending
- **Up/Down**: ascending then descending, excluding endpoints on reversal
- **Random**: `xorshift32` RNG with duplicate avoidance (rerolls if same as last note)
- **As Played**: preserves insertion order

**Octave spread**: the base note sequence is replicated across 1–4 octaves (+12 semitones per octave).

**Latch**: when enabled, `noteOff()` does not remove notes from the sequence. New `noteOn()` events clear the latched set and start fresh.
