#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

class Mixer
{
public:
    void setOsc1Level (float level) { osc1Level = level; }
    void setOsc2Level (float level) { osc2Level = level; }
    void setSubLevel (float level)  { subLevel = level; }
    void setNoiseLevel (float level) { noiseLevel = level; }

    float getOsc1Level() const { return osc1Level; }
    float getOsc2Level() const { return osc2Level; }
    float getSubLevel() const  { return subLevel; }
    float getNoiseLevel() const { return noiseLevel; }

    void mixBlock (const float* osc1, const float* osc2,
                   const float* sub, const float* noise,
                   float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = osc1[i] * osc1Level
                         + osc2[i] * osc2Level
                         + sub[i]  * subLevel
                         + noise[i] * noiseLevel;

            // Soft clip the mixer output to prevent harsh peaks
            output[i] = std::tanh (sample);
        }
    }

private:
    float osc1Level = 0.8f;
    float osc2Level = 0.0f;
    float subLevel  = 0.0f;
    float noiseLevel = 0.0f;
};
