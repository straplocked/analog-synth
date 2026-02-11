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
#include <algorithm>
#include "FastMath.h"

// TB-303 style acid filter — 2-pole diode ladder with distinctive resonance
// Sharper resonance curve, accent-sensitive, bass-heavy character
class AcidFilter
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void setCutoff (float frequencyHz)
    {
        cutoff = std::clamp (frequencyHz, 20.0f, 20000.0f);
        updateCoefficients();
    }

    void setResonance (float res)
    {
        // 2-pole feedback range: 0 to ~4 (self-oscillation near 4)
        resonance = std::clamp (res, 0.0f, 1.0f) * 4.0f;
    }

    void setDrive (float driveAmount)
    {
        drive = std::max (1.0f, driveAmount);
    }

    float processSample (float input)
    {
        // Acid-style input saturation (keeps 303 character)
        float drivenInput = acidClip (input * drive);

        // Makeup gain (1.5x) compensates for acidClip's heavy attenuation
        // (acidClip(1.0)=0.5 vs tanh(1.0)=0.76 — 1.5x brings it to 0.75, near parity)
        drivenInput *= 1.5f / (1.0f + resonance * 0.05f);

        // Feedback with acid saturation (keeps the 303 squelch)
        float feedback = resonance * acidClip (stage[1]);

        float u = drivenInput - feedback;

        // TPT stages with tanh saturation (not acidClip — avoids triple compression)
        for (int i = 0; i < 2; ++i)
        {
            float v = G * (std::tanh (u) - state[i]);
            float y = v + state[i];
            state[i] = y + v;
            stage[i] = y;
            u = y;
        }

        // DC blocker
        float out = stage[1];
        float dcBlocked = out - dcState + 0.9999f * dcPrev;
        dcState = out;
        dcPrev = dcBlocked;

        return dcBlocked;
    }

    void reset()
    {
        for (int i = 0; i < 2; ++i)
        {
            stage[i] = 0.0f;
            state[i] = 0.0f;
        }
        dcState = 0.0f;
        dcPrev = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float cutoff = 8000.0f;
    float resonance = 0.0f;
    float drive = 1.0f;

    // TPT coefficients
    float g = 0.0f;
    float G = 0.0f;

    // Two filter stages
    float stage[2] = {};
    float state[2] = {};

    // DC blocker state
    float dcState = 0.0f;
    float dcPrev = 0.0f;

    // Acid-style algebraic soft clip — sharper than tanh
    static float acidClip (float x)
    {
        return x / (1.0f + std::abs (x));
    }

    void updateCoefficients()
    {
        float wd = 2.0f * static_cast<float> (M_PI) * cutoff;
        float T = 1.0f / static_cast<float> (sampleRate);
        g = FastMath::tan (wd * T / 2.0f);
        g = std::min (g, 10.0f);
        G = g / (1.0f + g);
    }
};
