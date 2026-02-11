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
#include "../DSP/Saturation.h"
#include <cmath>

class DistortionEffect : public FXBase
{
public:
    DistortionEffect() = default;

    void prepare (double sampleRate, int /*samplesPerBlock*/) override
    {
        toneCoeff = 0.0f;
        toneState[0] = 0.0f;
        toneState[1] = 0.0f;
        sr = sampleRate;
        updateToneFilter();
    }

    void processBlock (juce::AudioBuffer<float>& buffer) override
    {
        if (! enabled) return;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float dry = data[i];
                float wet = applyDistortion (data[i]);

                // Tone filter (one-pole LP)
                toneState[ch] += toneCoeff * (wet - toneState[ch]);
                wet = wet * tone + toneState[ch] * (1.0f - tone);

                data[i] = dry * (1.0f - mix) + wet * mix;
            }
        }
    }

    void reset() override
    {
        toneState[0] = 0.0f;
        toneState[1] = 0.0f;
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::distEnabled)->load() > 0.5f);
        distType = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::distType)->load());
        drive = apvts.getRawParameterValue (ParameterIDs::distDrive)->load();
        tone = apvts.getRawParameterValue (ParameterIDs::distTone)->load();
        mix = apvts.getRawParameterValue (ParameterIDs::distMix)->load();
        updateToneFilter();
    }

    juce::String getEffectID() const override { return "distortion"; }
    juce::String getDisplayName() const override { return "DIST"; }

private:
    int distType = 0;
    float drive = 1.0f;
    float tone = 0.5f;
    float mix = 1.0f;
    double sr = 44100.0;

    float toneCoeff = 0.0f;
    float toneState[2] = {};

    float applyDistortion (float x) const
    {
        switch (distType)
        {
            case 0: // Tanh (original)
                return std::tanh (x * drive);

            case 1: // Tube
                return Saturation::warmSaturate (x, drive);

            case 2: // Hard Clip
                return juce::jlimit (-1.0f, 1.0f, x * drive);

            case 3: // Fuzz
            {
                float d = x * drive;
                return (d >= 0.0f ? 1.0f : -1.0f) * (1.0f - std::exp (-std::abs (d)));
            }

            case 4: // Bitcrush
            {
                float levels = std::pow (2.0f, 16.0f - drive * 0.75f);
                return std::round (x * levels) / levels;
            }

            case 5: // Tape
                return Saturation::tapeStyle (x, drive);

            case 6: // Wavefolder
                return std::sin (x * drive * static_cast<float> (M_PI));

            default:
                return std::tanh (x * drive);
        }
    }

    void updateToneFilter()
    {
        // Map tone 0-1 to frequency 200Hz - 10kHz
        float freq = 200.0f + tone * 9800.0f;
        toneCoeff = 1.0f - std::exp (-2.0f * static_cast<float> (M_PI) * freq / static_cast<float> (sr));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionEffect)
};
