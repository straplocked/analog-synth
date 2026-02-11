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

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

class Mixer
{
public:
    void setOsc1Level (float level) { osc1Level = level; }
    void setOsc2Level (float level) { osc2Level = level; }
    void setOsc3Level (float level) { osc3Level = level; }
    void setSubLevel (float level)  { subLevel = level; }
    void setNoiseLevel (float level) { noiseLevel = level; }

    float getOsc1Level() const { return osc1Level; }
    float getOsc2Level() const { return osc2Level; }
    float getOsc3Level() const { return osc3Level; }
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
    float osc3Level = 0.0f;
    float subLevel  = 0.0f;
    float noiseLevel = 0.0f;
};
