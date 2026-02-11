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

class FlangerEffect : public FXBase
{
public:
    FlangerEffect() = default;

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;

        for (int ch = 0; ch < 2; ++ch)
        {
            delayLine[ch].reset();
            delayLine[ch].setMaximumDelayInSamples (static_cast<int> (sampleRate * 0.02)); // 20ms max
        }

        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
        for (int ch = 0; ch < 2; ++ch)
            delayLine[ch].prepare (spec);

        lfoPhase[0] = 0.0;
        lfoPhase[1] = 0.25; // 90 degrees offset for stereo

        feedbackState[0] = 0.0f;
        feedbackState[1] = 0.0f;
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

                // Sinusoidal LFO modulation
                float lfoVal = std::sin (static_cast<float> (lfoPhase[ch] * 2.0 * M_PI));

                // Base delay 0.5ms + modulated depth up to 5ms
                float delaySamples = (baseDelay + depth * modDepthRange * (lfoVal * 0.5f + 0.5f)) * static_cast<float> (sr);
                delaySamples = std::max (1.0f, delaySamples);

                // Feed input + feedback into delay line
                float inputSample = dry + feedbackState[ch] * feedback;
                delayLine[ch].pushSample (ch, inputSample);
                float wet = delayLine[ch].popSample (ch, delaySamples);

                // Store delayed output for feedback on next sample
                feedbackState[ch] = wet;

                // Mix dry and wet
                data[i] = dry * (1.0f - mix) + wet * mix;
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
        for (int ch = 0; ch < 2; ++ch)
            delayLine[ch].reset();
        lfoPhase[0] = 0.0;
        lfoPhase[1] = 0.25;
        feedbackState[0] = 0.0f;
        feedbackState[1] = 0.0f;
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::flangerEnabled)->load() > 0.5f);
        rate = apvts.getRawParameterValue (ParameterIDs::flangerRate)->load();
        depth = apvts.getRawParameterValue (ParameterIDs::flangerDepth)->load();
        feedback = apvts.getRawParameterValue (ParameterIDs::flangerFeedback)->load();
        mix = apvts.getRawParameterValue (ParameterIDs::flangerMix)->load();
    }

    juce::String getEffectID() const override { return "flanger"; }
    juce::String getDisplayName() const override { return "FLANGER"; }

private:
    static constexpr float baseDelay = 0.0005f;      // 0.5ms base delay
    static constexpr float modDepthRange = 0.005f;    // up to 5ms modulation depth

    float rate = 0.5f;
    float depth = 0.5f;
    float feedback = 0.0f;
    float mix = 0.5f;
    double sr = 44100.0;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine[2];
    double lfoPhase[2] = {};
    float feedbackState[2] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlangerEffect)
};
