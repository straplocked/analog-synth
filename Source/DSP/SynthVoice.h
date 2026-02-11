/*
 * AnalogSynth - Virtual analog synthesizer plugin
 * Copyright (C) 2025 AnalogSynth contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "PolyBLEPOscillator.h"
#include "SubOscillator.h"
#include "NoiseGenerator.h"
#include "Mixer.h"
#include "ADSREnvelope.h"
#include "FilterEngine.h"
#include "StateVariableHPF.h"
#include "LFO.h"
#include "PolyModRouter.h"
#include "ModMatrix.h"
#include "PinkNoise.h"
#include "Saturation.h"

class SynthVoice
{
public:
    static constexpr int kOversampleFactor = 2;

    void prepare (double sampleRate, int samplesPerBlock);

    void noteOn (int midiNote, float velocity);
    void noteOff();

    bool isActive() const { return ampEnv.isActive(); }
    int getCurrentNote() const { return currentNote; }

    // Oscillators
    void setOsc1Waveform (int waveformIndex);
    void setOsc2Waveform (int waveformIndex);
    void setOsc3Waveform (int waveformIndex);
    void setOsc1PulseWidth (float pw);
    void setOsc2PulseWidth (float pw);
    void setOsc3PulseWidth (float pw);
    void setOsc1Tuning (int octave, int semitone, float fine);
    void setOsc2Tuning (int octave, int semitone, float fine);
    void setOsc3Tuning (int octave, int semitone, float fine);
    void setMixerLevels (float osc1, float osc2, float osc3, float sub, float noise);

    // Sub oscillator
    void setSubWaveform (int waveformIndex);
    void setSubOctave (int octaveChoice);
    void setNoiseType (int type) { noise.setNoiseType (type); }

    // Filter
    void setFilterCutoff (float hz);
    void setFilterResonance (float res);
    void setFilterEnvAmount (float amount);
    void setFilterKeyFollow (float amount);
    void setFilterDrive (float driveAmount);
    void setHPFCutoff (float hz);

    // Envelopes
    void setFilterEnvParams (float attack, float hold, float decay, float sustain, float release);
    void setAmpEnvParams (float attack, float hold, float decay, float sustain, float release);

    // LFO 1
    void setLFOShape (int shapeIndex);
    void setLFORate (float hz);
    void setLFOKeySync (bool enabled);
    void setLFOStepSmooth (bool enabled) { lfo.setStepSmooth (enabled); }
    void setLFOCustomPoint (int index, float x, float y) { lfo.setCustomPoint (index, x, y); }
    void setLFOCustomNumPoints (int n) { lfo.setCustomNumPoints (n); }
    void setLFOStepValue (int index, float value) { lfo.setStepValue (index, value); }
    void setLFOStepSeqNumSteps (int n) { lfo.setStepSeqNumSteps (n); }
    void setLFOAmounts (const LFOAmounts& amounts);

    // LFO 2
    void setLFO2Shape (int shapeIndex);
    void setLFO2Rate (float hz);
    void setLFO2KeySync (bool enabled);
    void setLFO2StepSmooth (bool enabled) { lfo2.setStepSmooth (enabled); }
    void setLFO2CustomPoint (int index, float x, float y) { lfo2.setCustomPoint (index, x, y); }
    void setLFO2CustomNumPoints (int n) { lfo2.setCustomNumPoints (n); }
    void setLFO2StepValue (int index, float value) { lfo2.setStepValue (index, value); }
    void setLFO2StepSeqNumSteps (int n) { lfo2.setStepSeqNumSteps (n); }

    // LFO 3
    void setLFO3Shape (int shapeIndex);
    void setLFO3Rate (float hz);
    void setLFO3KeySync (bool enabled);
    void setLFO3StepSmooth (bool enabled) { lfo3.setStepSmooth (enabled); }
    void setLFO3CustomPoint (int index, float x, float y) { lfo3.setCustomPoint (index, x, y); }
    void setLFO3CustomNumPoints (int n) { lfo3.setCustomNumPoints (n); }
    void setLFO3StepValue (int index, float value) { lfo3.setStepValue (index, value); }
    void setLFO3StepSeqNumSteps (int n) { lfo3.setStepSeqNumSteps (n); }

    // Envelope 3
    void setEnv3Params (float attack, float hold, float decay, float sustain, float release);
    void setEnv3Loop (bool enabled, int loopStart, int loopEnd)
    {
        env3.setLoopEnabled (enabled);
        env3.setLoopStart (loopStart);
        env3.setLoopEnd (loopEnd);
    }

    // Envelope 4
    void setEnv4Params (float attack, float hold, float decay, float sustain, float release);
    void setEnv4Loop (bool enabled, int loopStart, int loopEnd)
    {
        env4.setLoopEnabled (enabled);
        env4.setLoopStart (loopStart);
        env4.setLoopEnd (loopEnd);
    }

    // Poly Mod (Osc2 audio-rate FM — legacy, kept separate from matrix)
    void setPolyModAmounts (const PolyModAmounts& amounts);

    // Mod Matrix
    void setModMatrix (const ModMatrix* matrix) { modMatrix = matrix; }
    void setGlobalModValues (const GlobalModValues* vals) { globalModVals = vals; }

    // Hard sync
    void setSyncEnabled (bool enabled) { syncEnabled = enabled; }

    // Filter type and slope
    void setFilterType (int type) { filterEngine.setType (type); }
    void setFilterSlope (int slope) { filterEngine.setSlope (slope); }

    // Analog drift / imperfections
    void setPitchDriftCents (float cents) { pitchDriftCents = cents; }
    void setDCOffset (float offset) { dcOffset = offset; }
    void setSlopAmount (float amount) { slopAmount = amount; }

    // Portamento/Glide
    void setGlideEnabled (bool enabled) { glideEnabled = enabled; }
    void setGlideTime (float seconds);

    // Unison detune
    void setUnisonDetune (float cents) { unisonDetuneCents = cents; }

    // Per-voice LFO phase offset for natural modulation spread
    void setVoiceIndex (uint32_t index)
    {
        double offset = std::fmod (static_cast<double> (index) * 0.381966011, 1.0);
        lfo.setVoicePhaseOffset (offset);
        lfo2.setVoicePhaseOffset (std::fmod (offset + 0.217, 1.0));
        lfo3.setVoicePhaseOffset (std::fmod (offset + 0.618, 1.0));
    }

    // Phase randomization for unison voices
    void setStartPhase (float phase01)
    {
        osc1.setPhase (static_cast<double> (phase01));
        osc2.setPhase (static_cast<double> (std::fmod (phase01 + 0.37f, 1.0f)));
        osc3.setPhase (static_cast<double> (std::fmod (phase01 + 0.73f, 1.0f)));
        subOsc.setPhase (static_cast<double> (std::fmod (phase01 + 0.61f, 1.0f)));
    }

    // Legato: change pitch without retriggering envelopes
    void setNote (int midiNote);

    // Performance controllers
    void setPitchBend (float semitones) { pitchBendSemitones = semitones; }
    void setModWheel (float value)      { modWheelAmount = value; }

    void renderBlock (float* output, int numSamples);

private:
    PolyBLEPOscillator osc1;
    PolyBLEPOscillator osc2;
    PolyBLEPOscillator osc3;
    SubOscillator subOsc;
    NoiseGenerator noise;
    Mixer mixer;

    ADSREnvelope filterEnv;
    ADSREnvelope ampEnv;
    ADSREnvelope env3;
    ADSREnvelope env4;
    FilterEngine filterEngine;
    StateVariableHPF hpf;
    LFO lfo;
    LFO lfo2;
    LFO lfo3;

    PolyModAmounts polyMod;
    LFOAmounts lfoAmounts;

    // Mod matrix (owned by VoiceManager, shared across all voices)
    const ModMatrix* modMatrix = nullptr;
    const GlobalModValues* globalModVals = nullptr;

    // Per-note random value for mod matrix Random source
    float noteRandomValue = 0.0f;

    double sampleRate = 44100.0;
    int currentNote = -1;
    float currentVelocity = 0.0f;

    // Tuning offsets
    int osc1Octave = 0, osc1Semitone = 0;
    float osc1Fine = 0.0f;
    int osc2Octave = 0, osc2Semitone = 0;
    float osc2Fine = 0.0f;
    int osc3Octave = 0, osc3Semitone = 0;
    float osc3Fine = 0.0f;

    // Base oscillator frequencies (cached)
    float baseFreq1 = 440.0f;
    float baseFreq2 = 440.0f;
    float baseFreq3 = 440.0f;
    float basePW1 = 0.5f;

    // Filter parameters
    float baseCutoff = 8000.0f;
    float filterEnvAmount = 0.3f;
    float filterKeyFollow = 0.0f;
    float filterResonance = 0.0f;
    float filterDrive = 1.0f;
    float baseHpfCutoff = 20.0f;

    // Temp buffers
    juce::HeapBlock<float> osc1Buffer, osc2Buffer, osc3Buffer, subBuffer, noiseBuffer;
    int maxBlockSize = 0;

    bool lfoKeySync = false;
    bool lfo2KeySync = false;
    bool lfo3KeySync = false;

    // Hard sync
    bool syncEnabled = false;
    float prevOsc2Out = 0.0f;

    // Analog drift / imperfections
    float pitchDriftCents = 0.0f;
    float dcOffset = 0.0f;
    float slopAmount = 0.0f;
    PinkNoise phaseJitter1;
    PinkNoise phaseJitter2;
    PinkNoise phaseJitter3;
    PinkNoise noiseFloor;

    // Performance controllers
    float pitchBendSemitones = 0.0f;
    float modWheelAmount = 0.0f;

    // Portamento/Glide
    bool glideEnabled = false;
    float glideTimeSec = 0.1f;
    float glideCoeff = 0.0f;
    float currentFreq1 = 440.0f; // smoothed towards baseFreq1
    float currentFreq2 = 440.0f; // smoothed towards baseFreq2
    float currentFreq3 = 440.0f; // smoothed towards baseFreq3

    // Unison
    float unisonDetuneCents = 0.0f;

    // Oversampling
    double oversampledRate = 88200.0;
    juce::HeapBlock<float> oversampledBuffer;
    // 15-tap half-band FIR (Hamming window, ~43dB stopband, DC gain = 1.0)
    static constexpr int kFIRLen = 15;
    static constexpr int kFIRBufSize = 16;
    static constexpr int kFIRBufMask = kFIRBufSize - 1;
    static constexpr float halfBandFIR[kFIRLen] = {
        -0.003665f, 0.0f,  0.016237f, 0.0f, -0.068638f, 0.0f, 0.306066f,
         0.5f,
         0.306066f, 0.0f, -0.068638f, 0.0f,  0.016237f, 0.0f, -0.003665f
    };
    float firState[kFIRBufSize] = {};
    int firWritePos = 0;

    void renderOversampled (float* output, int oversampledSamples);

    float noteToFrequency (int midiNote, int octaveOffset, int semitoneOffset, float centsOffset) const;
    void updateOscillatorFrequencies();
};
