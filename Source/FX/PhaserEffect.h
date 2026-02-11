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

#include "FXBase.h"
#include "../ParameterIDs.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class PhaserEffect : public FXBase
{
public:
    PhaserEffect() = default;

    void prepare (double sampleRate, int /*samplesPerBlock*/) override
    {
        sr = sampleRate;
        reset();
    }

    void processBlock (juce::AudioBuffer<float>& buffer) override
    {
        if (! enabled) return;

        int numChannels = std::min (buffer.getNumChannels(), 2);
        int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                float dry = data[i];

                // LFO with 90-degree stereo offset
                float lfoVal = std::sin (static_cast<float> (lfoPhase[ch] * 2.0 * M_PI));

                // Map LFO to center frequency logarithmically (200 Hz - 4000 Hz)
                // depth controls how wide the sweep is around the center
                float logMin = std::log (200.0f);
                float logMax = std::log (4000.0f);
                float logCenter = (logMin + logMax) * 0.5f;
                float logRange = (logMax - logMin) * 0.5f * depth;
                float sweepFreq = std::exp (logCenter + lfoVal * logRange);

                // Calculate all-pass coefficient from swept frequency
                float tanVal = std::tan (static_cast<float> (M_PI) * sweepFreq / static_cast<float> (sr));
                float coeff = (tanVal - 1.0f) / (tanVal + 1.0f);

                // Input with feedback
                float input = dry + feedbackState[ch] * feedback;

                // Clamp feedback input to prevent runaway
                input = std::clamp (input, -4.0f, 4.0f);

                // Process through cascaded all-pass stages
                float y = input;
                for (int s = 0; s < numStages; ++s)
                {
                    float x = y;
                    y = -coeff * x + apStateX[ch][s] + coeff * apStateY[ch][s];
                    apStateX[ch][s] = x;
                    apStateY[ch][s] = y;
                }

                feedbackState[ch] = y;

                data[i] = dry * (1.0f - mix) + y * mix;
            }

            // Advance LFO phases
            double phaseInc = rate / sr;
            lfoPhase[0] += phaseInc;
            lfoPhase[1] += phaseInc;
            if (lfoPhase[0] >= 1.0) lfoPhase[0] -= 1.0;
            if (lfoPhase[1] >= 1.0) lfoPhase[1] -= 1.0;
        }
    }

    void reset() override
    {
        lfoPhase[0] = 0.0;
        lfoPhase[1] = 0.25; // 90 degrees offset for stereo

        for (int ch = 0; ch < 2; ++ch)
        {
            feedbackState[ch] = 0.0f;
            for (int s = 0; s < maxStages; ++s)
            {
                apStateX[ch][s] = 0.0f;
                apStateY[ch][s] = 0.0f;
            }
        }
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::phaserEnabled)->load() > 0.5f);
        rate = apvts.getRawParameterValue (ParameterIDs::phaserRate)->load();
        depth = apvts.getRawParameterValue (ParameterIDs::phaserDepth)->load();
        feedback = apvts.getRawParameterValue (ParameterIDs::phaserFeedback)->load();
        mix = apvts.getRawParameterValue (ParameterIDs::phaserMix)->load();

        int stagesIndex = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::phaserStages)->load());
        // Choice param values: 0 = "4", 1 = "6", 2 = "8"
        switch (stagesIndex)
        {
            case 0:  numStages = 4; break;
            case 1:  numStages = 6; break;
            case 2:  numStages = 8; break;
            default: numStages = 4; break;
        }
    }

    juce::String getEffectID() const override { return "phaser"; }
    juce::String getDisplayName() const override { return "PHASER"; }

private:
    static constexpr int maxStages = 8;

    float rate = 0.5f;
    float depth = 0.5f;
    float feedback = 0.0f;
    float mix = 0.5f;
    int numStages = 4;
    double sr = 44100.0;

    double lfoPhase[2] = {};
    float feedbackState[2] = {};
    float apStateX[2][maxStages] = {};
    float apStateY[2][maxStages] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaserEffect)
};
