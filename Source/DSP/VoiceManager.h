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
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "SynthVoice.h"
#include "AnalogDrift.h"
#include "ModMatrix.h"
#include "ChaosOscillator.h"

class VoiceManager
{
    juce::Random rng;


public:
    static constexpr int kMaxVoices = 6;

    void prepare (double sampleRate, int samplesPerBlock);

    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();

    void renderBlock (juce::AudioBuffer<float>& outputBuffer, int numSamples);

    void updateParameters (juce::AudioProcessorValueTreeState& apvts);
    void updateLFOShapeData (juce::AudioProcessorValueTreeState& apvts);

    void setPitchBend (float normalised);  // -1 to +1
    void setModWheel (float value);        // 0 to 1
    void setAftertouch (float value);      // 0 to 1

    // Access mod values for FX modulation in processBlock
    const ModMatrix& getModMatrix() const { return modMatrix; }
    const GlobalModValues& getGlobalModValues() const { return globalModValues; }

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
    int numUnisonVoices = kMaxVoices;

    // Mod matrix and global mod values
    ModMatrix modMatrix;
    GlobalModValues globalModValues;
    float aftertouchVal = 0.0f;

    // Chaos oscillator (global modulation source)
    ChaosOscillator chaosOsc;

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
