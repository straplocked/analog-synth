#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "PolyBLEPOscillator.h"
#include "SubOscillator.h"
#include "NoiseGenerator.h"
#include "Mixer.h"
#include "ADSREnvelope.h"
#include "LadderFilter.h"
#include "DiodeLadderFilter.h"
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
    void setOsc1PulseWidth (float pw);
    void setOsc2PulseWidth (float pw);
    void setOsc1Tuning (int octave, int semitone, float fine);
    void setOsc2Tuning (int octave, int semitone, float fine);
    void setMixerLevels (float osc1, float osc2, float sub, float noise);

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

    // Filter type
    void setFilterType (int type) { filterType = type; }

    // Analog drift / imperfections
    void setPitchDriftCents (float cents) { pitchDriftCents = cents; }
    void setDCOffset (float offset) { dcOffset = offset; }
    void setSlopAmount (float amount) { slopAmount = amount; }

    // Portamento/Glide
    void setGlideEnabled (bool enabled) { glideEnabled = enabled; }
    void setGlideTime (float seconds);

    // Unison detune
    void setUnisonDetune (float cents) { unisonDetuneCents = cents; }

    // Legato: change pitch without retriggering envelopes
    void setNote (int midiNote);

    // Performance controllers
    void setPitchBend (float semitones) { pitchBendSemitones = semitones; }
    void setModWheel (float value)      { modWheelAmount = value; }

    void renderBlock (float* output, int numSamples);

private:
    PolyBLEPOscillator osc1;
    PolyBLEPOscillator osc2;
    SubOscillator subOsc;
    NoiseGenerator noise;
    Mixer mixer;

    ADSREnvelope filterEnv;
    ADSREnvelope ampEnv;
    LadderFilter lpf;
    DiodeLadderFilter diodeLpf;
    StateVariableHPF hpf;
    int filterType = 0; // 0=Ladder, 1=Diode
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

    // Base oscillator frequencies (cached)
    float baseFreq1 = 440.0f;
    float baseFreq2 = 440.0f;
    float basePW1 = 0.5f;

    // Filter parameters
    float baseCutoff = 8000.0f;
    float filterEnvAmount = 0.3f;
    float filterKeyFollow = 0.0f;
    float filterResonance = 0.0f;
    float filterDrive = 1.0f;
    float baseHpfCutoff = 20.0f;

    // Temp buffers
    juce::HeapBlock<float> osc1Buffer, osc2Buffer, subBuffer, noiseBuffer;
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
