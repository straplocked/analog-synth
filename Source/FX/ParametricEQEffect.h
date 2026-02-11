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

class ParametricEQEffect : public FXBase
{
public:
    ParametricEQEffect() = default;

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;

        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };

        for (auto& f : lowShelf)
            f.prepare (spec);
        for (auto& f : midPeak)
            f.prepare (spec);
        for (auto& f : highShelf)
            f.prepare (spec);

        updateFilters();
    }

    void processBlock (juce::AudioBuffer<float>& buffer) override
    {
        if (! enabled) return;

        updateFilters();

        int numSamples = buffer.getNumSamples();
        int numChannels = std::min (buffer.getNumChannels(), 2);

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                float sample = data[i];

                sample = lowShelf[ch].processSample (sample);
                sample = midPeak[ch].processSample (sample);
                sample = highShelf[ch].processSample (sample);

                data[i] = sample;
            }
        }
    }

    void reset() override
    {
        for (auto& f : lowShelf) f.reset();
        for (auto& f : midPeak) f.reset();
        for (auto& f : highShelf) f.reset();
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::eqEnabled)->load() > 0.5f);
        lowFreq  = apvts.getRawParameterValue (ParameterIDs::eqLowFreq)->load();
        lowGain  = apvts.getRawParameterValue (ParameterIDs::eqLowGain)->load();
        midFreq  = apvts.getRawParameterValue (ParameterIDs::eqMidFreq)->load();
        midGain  = apvts.getRawParameterValue (ParameterIDs::eqMidGain)->load();
        midQ     = apvts.getRawParameterValue (ParameterIDs::eqMidQ)->load();
        highFreq = apvts.getRawParameterValue (ParameterIDs::eqHighFreq)->load();
        highGain = apvts.getRawParameterValue (ParameterIDs::eqHighGain)->load();
    }

    juce::String getEffectID() const override { return "eq"; }
    juce::String getDisplayName() const override { return "EQ"; }

private:
    float lowFreq  = 200.0f;
    float lowGain  = 0.0f;
    float midFreq  = 1000.0f;
    float midGain  = 0.0f;
    float midQ     = 1.0f;
    float highFreq = 5000.0f;
    float highGain = 0.0f;
    double sr = 44100.0;

    // Per-channel IIR filters (biquad)
    juce::dsp::IIR::Filter<float> lowShelf[2];
    juce::dsp::IIR::Filter<float> midPeak[2];
    juce::dsp::IIR::Filter<float> highShelf[2];

    void updateFilters()
    {
        if (sr <= 0.0) return;

        // Clamp frequencies to safe range
        float safeNyquist = static_cast<float> (sr) * 0.49f;

        float lf = std::min (lowFreq, safeNyquist);
        float mf = std::min (midFreq, safeNyquist);
        float hf = std::min (highFreq, safeNyquist);

        auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            sr, lf, 0.707f, juce::Decibels::decibelsToGain (lowGain));
        auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sr, mf, midQ, juce::Decibels::decibelsToGain (midGain));
        auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sr, hf, 0.707f, juce::Decibels::decibelsToGain (highGain));

        for (auto& f : lowShelf)
            *f.coefficients = *lowCoeffs;
        for (auto& f : midPeak)
            *f.coefficients = *midCoeffs;
        for (auto& f : highShelf)
            *f.coefficients = *highCoeffs;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametricEQEffect)
};
