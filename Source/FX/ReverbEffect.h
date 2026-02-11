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

class ReverbEffect : public FXBase
{
public:
    ReverbEffect() = default;

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
        reverb.prepare (spec);
        sr = sampleRate;

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 20000.0f);
        dampingFilterL.coefficients = coeffs;
        dampingFilterR.coefficients = coeffs;
        dampingFilterL.reset();
        dampingFilterR.reset();

        // Pre-delay: up to 100ms
        int maxDelaySamples = static_cast<int> (sampleRate * 0.1) + 1;
        juce::dsp::ProcessSpec monoSpec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 1 };
        for (int ch = 0; ch < 2; ++ch)
        {
            preDelayLine[ch].setMaximumDelayInSamples (maxDelaySamples);
            preDelayLine[ch].prepare (monoSpec);
        }

        dryBuffer.setSize (2, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer) override
    {
        if (! enabled) return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        // Save dry signal
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        // Apply pre-delay before reverb
        if (preDelayMs > 0.0f)
        {
            float delaySamples = preDelayMs * 0.001f * static_cast<float> (sr);
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    preDelayLine[ch].pushSample (0, data[i]);
                    data[i] = preDelayLine[ch].popSample (0, delaySamples);
                }
            }
        }

        // Process reverb (100% wet internally)
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        reverb.process (context);

        // Apply damping lowpass to wet signal
        auto* leftWet = buffer.getWritePointer (0);
        for (int i = 0; i < numSamples; ++i)
            leftWet[i] = dampingFilterL.processSample (leftWet[i]);

        if (numChannels > 1)
        {
            auto* rightWet = buffer.getWritePointer (1);
            for (int i = 0; i < numSamples; ++i)
                rightWet[i] = dampingFilterR.processSample (rightWet[i]);
        }

        // Mix dry + filtered wet
        const float dry = 1.0f - wetLevel;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* out = buffer.getWritePointer (ch);
            const auto* dryData = dryBuffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
                out[i] = dryData[i] * dry + out[i] * wetLevel;
        }
    }

    void reset() override
    {
        reverb.reset();
        dampingFilterL.reset();
        dampingFilterR.reset();
        for (int ch = 0; ch < 2; ++ch)
            preDelayLine[ch].reset();
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::reverbEnabled)->load() > 0.5f);
        reverbType = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::reverbType)->load());
        size = apvts.getRawParameterValue (ParameterIDs::reverbSize)->load();
        damping = apvts.getRawParameterValue (ParameterIDs::reverbDamping)->load();
        wetLevel = apvts.getRawParameterValue (ParameterIDs::reverbMix)->load();
        preDelayMs = apvts.getRawParameterValue (ParameterIDs::reverbPreDelay)->load();
        updateParams();
    }

    juce::String getEffectID() const override { return "reverb"; }
    juce::String getDisplayName() const override { return "REVERB"; }

private:
    int reverbType = 0;
    float size = 0.5f;
    float damping = 0.5f;
    float wetLevel = 0.2f;
    double sr = 44100.0;

    juce::dsp::Reverb reverb;
    juce::dsp::IIR::Filter<float> dampingFilterL, dampingFilterR;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayLine[2];
    float preDelayMs = 10.0f;
    juce::AudioBuffer<float> dryBuffer;

    void updateParams()
    {
        juce::dsp::Reverb::Parameters params;

        switch (reverbType)
        {
            case 0: // Room
                params.roomSize = 0.2f + size * 0.4f;
                break;
            case 1: // Hall
                params.roomSize = 0.5f + size * 0.5f;
                break;
            case 2: // Plate
                params.roomSize = 0.4f + size * 0.4f;
                break;
            case 3: // Spring
                params.roomSize = 0.3f + size * 0.3f;
                break;
            default:
                break;
        }

        params.damping = damping;
        params.wetLevel = 1.0f;
        params.dryLevel = 0.0f;
        params.width = 1.0f;
        params.freezeMode = 0.0f;

        reverb.setParameters (params);

        // Map damping 0 -> 20kHz (bright) to 1 -> 800Hz (very dark)
        float cutoffHz = 20000.0f * std::pow (800.0f / 20000.0f, damping);
        cutoffHz = juce::jlimit (200.0f, 20000.0f, cutoffHz);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, cutoffHz);
        *dampingFilterL.coefficients = *coeffs;
        *dampingFilterR.coefficients = *coeffs;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbEffect)
};
