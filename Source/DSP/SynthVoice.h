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

    // Filter
    void setFilterCutoff (float hz);
    void setFilterResonance (float res);
    void setFilterEnvAmount (float amount);
    void setFilterKeyFollow (float amount);
    void setFilterDrive (float driveAmount);
    void setHPFCutoff (float hz);

    // Envelopes
    void setFilterEnvParams (float attack, float decay, float sustain, float release);
    void setAmpEnvParams (float attack, float decay, float sustain, float release);

    // LFO
    void setLFOShape (int shapeIndex);
    void setLFORate (float hz);
    void setLFOKeySync (bool enabled);
    void setLFOAmounts (const LFOAmounts& amounts);

    // Poly Mod
    void setPolyModAmounts (const PolyModAmounts& amounts);

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
    FilterEngine filterEngine;
    StateVariableHPF hpf;
    LFO lfo;

    PolyModAmounts polyMod;
    LFOAmounts lfoAmounts;

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
    static constexpr int kFIRLen = 7;
    static constexpr float halfBandFIR[kFIRLen] = { -0.045f, 0.0f, 0.295f, 0.5f, 0.295f, 0.0f, -0.045f };
    float firState[kFIRLen] = {};

    void renderOversampled (float* output, int oversampledSamples);

    float noteToFrequency (int midiNote, int octaveOffset, int semitoneOffset, float centsOffset) const;
    void updateOscillatorFrequencies();
};
