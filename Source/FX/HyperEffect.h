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

class HyperEffect : public FXBase
{
public:
    HyperEffect() = default;

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;

        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 1 };
        int maxDelaySamples = static_cast<int> (sampleRate * 0.01); // 10ms max

        for (int v = 0; v < maxVoices; ++v)
        {
            delayLines[v].reset();
            delayLines[v].setMaximumDelayInSamples (maxDelaySamples);
            delayLines[v].prepare (spec);
        }

        // Initialize LFO phases evenly spread across 0..1
        for (int v = 0; v < maxVoices; ++v)
            lfoPhases[v] = static_cast<double> (v) / static_cast<double> (maxVoices);
    }

    void processBlock (juce::AudioBuffer<float>& buffer) override
    {
        if (! enabled) return;

        int numChannels = std::min (buffer.getNumChannels(), 2);
        int numSamples = buffer.getNumSamples();

        if (numChannels < 2) return; // need stereo

        float* dataL = buffer.getWritePointer (0);
        float* dataR = buffer.getWritePointer (1);

        // Number of active voices: 2, 4, or 8 (from choice parameter index 0, 1, 2)
        int activeVoices = voiceCount;

        // Base delay times for each voice, spread evenly from 0.5ms to 5ms
        // Detune amount in seconds (micro-detune via delay modulation)
        // 50 cents at 440Hz ~ 1.3% pitch shift. For a ~3ms delay, modulation depth
        // of ~0.0001s per cent gives perceptible detuning.
        float detuneDepthSec = detune * 0.000002f; // scale detune (0-50) to modulation depth

        // Slow LFO rate for micro-detune modulation (each voice slightly different)
        double baseLfoRate = 0.3; // Hz — slow, subtle modulation

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = dataL[i];
            float dryR = dataR[i];
            float mono = (dryL + dryR) * 0.5f;

            float wetL = 0.0f;
            float wetR = 0.0f;

            for (int v = 0; v < activeVoices; ++v)
            {
                // Each voice has a unique base delay time spread from 0.5ms to 5ms
                float baseDelaySec = 0.0005f + (0.0045f * static_cast<float> (v) / static_cast<float> (std::max (activeVoices - 1, 1)));

                // LFO modulation for micro-detune (each voice has its own phase)
                float lfoVal = std::sin (static_cast<float> (lfoPhases[v] * 2.0 * M_PI));
                float modulatedDelaySec = baseDelaySec + detuneDepthSec * lfoVal;
                modulatedDelaySec = std::max (modulatedDelaySec, 0.0001f);

                float delaySamples = modulatedDelaySec * static_cast<float> (sr);
                delaySamples = std::max (1.0f, delaySamples);

                // Push mono input into each voice's delay line (channel 0)
                delayLines[v].pushSample (0, mono);
                float wet = delayLines[v].popSample (0, delaySamples);

                // Pan: odd voices (0, 2, 4, 6) go left, even voices (1, 3, 5, 7) go right
                // spread controls the stereo width: 0 = center, 1 = full L/R separation
                float pan = (v % 2 == 0) ? -spread : spread; // -1..+1 range scaled by spread
                float gainL = std::cos ((pan + 1.0f) * 0.25f * static_cast<float> (M_PI));
                float gainR = std::sin ((pan + 1.0f) * 0.25f * static_cast<float> (M_PI));

                wetL += wet * gainL;
                wetR += wet * gainR;
            }

            // Normalize by number of voices to keep level consistent
            float voiceGain = 1.0f / std::sqrt (static_cast<float> (activeVoices));
            wetL *= voiceGain;
            wetR *= voiceGain;

            // Mix dry and wet
            dataL[i] = dryL * (1.0f - mix) + wetL * mix;
            dataR[i] = dryR * (1.0f - mix) + wetR * mix;

            // Advance LFO phases (each voice has slightly different rate for decorrelation)
            for (int v = 0; v < activeVoices; ++v)
            {
                double rate = baseLfoRate * (1.0 + 0.15 * static_cast<double> (v));
                lfoPhases[v] += rate / sr;
                if (lfoPhases[v] >= 1.0)
                    lfoPhases[v] -= 1.0;
            }
        }
    }

    void reset() override
    {
        for (int v = 0; v < maxVoices; ++v)
            delayLines[v].reset();

        for (int v = 0; v < maxVoices; ++v)
            lfoPhases[v] = static_cast<double> (v) / static_cast<double> (maxVoices);
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::hyperEnabled)->load() > 0.5f);

        int voicesChoice = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::hyperVoices)->load());
        switch (voicesChoice)
        {
            case 0:  voiceCount = 2; break;
            case 1:  voiceCount = 4; break;
            case 2:  voiceCount = 8; break;
            default: voiceCount = 2; break;
        }

        detune = apvts.getRawParameterValue (ParameterIDs::hyperDetune)->load();
        spread = apvts.getRawParameterValue (ParameterIDs::hyperSpread)->load();
        mix    = apvts.getRawParameterValue (ParameterIDs::hyperMix)->load();
    }

    juce::String getEffectID() const override { return "hyper"; }
    juce::String getDisplayName() const override { return "HYPER"; }

private:
    static constexpr int maxVoices = 8;

    int voiceCount = 2;
    float detune = 10.0f;
    float spread = 0.5f;
    float mix = 0.5f;
    double sr = 44100.0;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLines[maxVoices];
    double lfoPhases[maxVoices] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HyperEffect)
};
