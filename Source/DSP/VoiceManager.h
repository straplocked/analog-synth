#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "SynthVoice.h"
#include "AnalogDrift.h"

class VoiceManager
{
public:
    static constexpr int kMaxVoices = 6;

    void prepare (double sampleRate, int samplesPerBlock);

    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();

    void renderBlock (juce::AudioBuffer<float>& outputBuffer, int numSamples);

    void updateParameters (juce::AudioProcessorValueTreeState& apvts);

    void setPitchBend (float normalised);  // -1 to +1
    void setModWheel (float value);        // 0 to 1

    int getActiveVoiceCount() const;

private:
    SynthVoice voices[kMaxVoices];
    AnalogDrift drift[kMaxVoices];
    int voiceAge[kMaxVoices] = {};
    int nextAge = 0;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    float slopAmount = 0.2f;
    float pitchBendNorm = 0.0f;   // -1 to +1
    float modWheelVal = 0.0f;     // 0 to 1

    // Mono mode
    bool monoMode = false;
    std::vector<int> heldNotes;

    // Unison mode
    bool unisonEnabled = false;
    float unisonDetuneCents = 15.0f;
    float unisonSpread = 1.0f;

    // Temp buffer for summing voices
    juce::HeapBlock<float> voiceBuffer;

    int findFreeVoice() const;
    int findVoiceToSteal() const;
    int findVoicePlaying (int midiNote) const;

    // Poly mode note handling
    void noteOnPoly (int midiNote, float velocity);
    void noteOffPoly (int midiNote);

    // Mono mode note handling
    void noteOnMono (int midiNote, float velocity);
    void noteOffMono (int midiNote);

    // Unison mode note handling
    void noteOnUnison (int midiNote, float velocity);
    void noteOffUnison (int midiNote);
};
