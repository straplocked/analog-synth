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
#include <cstdint>
#include <algorithm>
#include <array>

class LFO
{
public:
    enum class Shape { Triangle, Saw, Square, Random, Sine, Custom, StepSeq };

    static constexpr int kMaxCustomPoints = 32;
    static constexpr int kMaxStepSeqSteps = 16;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        phase = voicePhaseOffset;
        randomValue = 0.0f;
        prevPhase = voicePhaseOffset;
        slewState = 0.0f;
        // 2ms slew time: smooths clicks without removing punch
        slewCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (newSampleRate) * 0.002f));
    }

    void setShape (int shapeIndex) { shape = static_cast<Shape> (shapeIndex); }
    void setRate (float hz) { rate = hz; }
    void setKeySync (bool enabled) { keySync = enabled; }
    void setStepSmooth (bool enabled) { stepSmooth = enabled; }

    void resetPhase() { phase = voicePhaseOffset; }

    void setVoicePhaseOffset (double offset)
    {
        voicePhaseOffset = std::fmod (std::abs (offset), 1.0);
    }

    // ---- Custom shape editing ----

    void setCustomNumPoints (int n) { customNumPoints = std::clamp (n, 2, kMaxCustomPoints); }
    int getCustomNumPoints() const { return customNumPoints; }

    void setCustomPoint (int index, float x, float y)
    {
        if (index >= 0 && index < kMaxCustomPoints)
        {
            customPointsX[index] = std::clamp (x, 0.0f, 1.0f);
            customPointsY[index] = std::clamp (y, -1.0f, 1.0f);
        }
    }

    // ---- Step sequencer editing ----

    void setStepSeqNumSteps (int n) { stepSeqNumSteps = std::clamp (n, 1, kMaxStepSeqSteps); }
    int getStepSeqNumSteps() const { return stepSeqNumSteps; }

    void setStepValue (int index, float value)
    {
        if (index >= 0 && index < kMaxStepSeqSteps)
            stepValues[index] = std::clamp (value, -1.0f, 1.0f);
    }

    float getStepValue (int index) const
    {
        if (index >= 0 && index < kMaxStepSeqSteps)
            return stepValues[index];
        return 0.0f;
    }

    // Returns bipolar output [-1, +1]
    float processSample()
    {
        float output = 0.0f;
        bool needsSlew = false;

        switch (shape)
        {
            case Shape::Triangle:
                if (phase < 0.5)
                    output = static_cast<float> (4.0 * phase - 1.0);
                else
                    output = static_cast<float> (3.0 - 4.0 * phase);
                break;

            case Shape::Saw:
                // Ramp down
                output = static_cast<float> (1.0 - 2.0 * phase);
                break;

            case Shape::Square:
                output = (phase < 0.5) ? 1.0f : -1.0f;
                needsSlew = true;
                break;

            case Shape::Random:
            {
                // Sample-and-hold: new random value at each cycle start
                if (phase < prevPhase) // wrapped around
                {
                    // Simple xorshift for random
                    randomState ^= randomState << 13;
                    randomState ^= randomState >> 17;
                    randomState ^= randomState << 5;
                    randomValue = static_cast<float> (static_cast<int32_t> (randomState))
                                / static_cast<float> (INT32_MAX);
                }
                output = randomValue;
                needsSlew = true;
                break;
            }

            case Shape::Sine:
                output = static_cast<float> (std::sin (phase * 2.0 * M_PI));
                break;

            case Shape::Custom:
                output = evaluateCustomShape (static_cast<float> (phase));
                break;

            case Shape::StepSeq:
                output = evaluateStepSeq (static_cast<float> (phase));
                needsSlew = stepSmooth;
                if (! stepSmooth)
                {
                    // Without smoothing, still apply slew to avoid harsh clicks
                    needsSlew = true;
                }
                break;
        }

        prevPhase = phase;
        phase += rate / sampleRate;
        if (phase >= 1.0)
            phase -= 1.0;

        // Apply one-pole slew to discontinuous waveforms (analog capacitor slew)
        if (needsSlew)
        {
            slewState += slewCoeff * (output - slewState);
            return slewState;
        }

        return output;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double prevPhase = 0.0;
    float rate = 2.0f;
    Shape shape = Shape::Triangle;
    bool keySync = false;

    // For S&H random mode
    uint32_t randomState = 54321;
    float randomValue = 0.0f;

    // Per-voice phase offset for natural modulation spread
    double voicePhaseOffset = 0.0;

    // Slew rate limiter for Square/S&H (analog capacitor slew)
    float slewState = 0.0f;
    float slewCoeff = 0.0f;

    // ---- Custom shape data ----
    // Points sorted by X (0.0 to 1.0), Y is bipolar (-1 to +1)
    // Default: linear ramp from -1 to +1
    int customNumPoints = 2;
    float customPointsX[kMaxCustomPoints] = { 0.0f, 1.0f };
    float customPointsY[kMaxCustomPoints] = { -1.0f, 1.0f };

    // ---- Step sequencer data ----
    int stepSeqNumSteps = 16;
    float stepValues[kMaxStepSeqSteps] = {};
    bool stepSmooth = false;

    float evaluateCustomShape (float p) const
    {
        if (customNumPoints < 2) return 0.0f;

        // Find the segment containing phase p
        for (int i = 0; i < customNumPoints - 1; ++i)
        {
            if (p <= customPointsX[i + 1] || i == customNumPoints - 2)
            {
                float x0 = customPointsX[i];
                float x1 = customPointsX[i + 1];
                float y0 = customPointsY[i];
                float y1 = customPointsY[i + 1];

                float dx = x1 - x0;
                if (dx < 0.0001f) return y0;

                float t = (p - x0) / dx;
                return y0 + t * (y1 - y0);
            }
        }

        return 0.0f;
    }

    float evaluateStepSeq (float p) const
    {
        int step = static_cast<int> (p * stepSeqNumSteps);
        step = std::clamp (step, 0, stepSeqNumSteps - 1);
        return stepValues[step];
    }
};
