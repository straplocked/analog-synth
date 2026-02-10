#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class DistortionEffect
{
public:
    void prepare (double sampleRate, int /*samplesPerBlock*/)
    {
        // Simple one-pole for tone control
        toneCoeff = 0.0f;
        updateToneFilter (sampleRate);
        sr = sampleRate;
    }

    void setEnabled (bool e) { enabled = e; }
    void setDrive (float d)  { drive = d; }
    void setTone (float t)   { tone = t; updateToneFilter (sr); }
    void setMix (float m)    { mix = m; }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        if (! enabled) return;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float dry = data[i];
                float wet = std::tanh (data[i] * drive);

                // Tone filter (one-pole LP)
                toneState[ch] += toneCoeff * (wet - toneState[ch]);
                wet = wet * tone + toneState[ch] * (1.0f - tone);

                data[i] = dry * (1.0f - mix) + wet * mix;
            }
        }
    }

private:
    bool enabled = false;
    float drive = 1.0f;
    float tone = 0.5f;
    float mix = 1.0f;
    double sr = 44100.0;

    float toneCoeff = 0.0f;
    float toneState[2] = {};

    void updateToneFilter (double sampleRate)
    {
        // Map tone 0-1 to frequency 200Hz - 10kHz
        float freq = 200.0f + tone * 9800.0f;
        toneCoeff = 1.0f - std::exp (-2.0f * static_cast<float> (M_PI) * freq / static_cast<float> (sampleRate));
    }
};
