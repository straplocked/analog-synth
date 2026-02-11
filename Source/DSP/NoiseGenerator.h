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

#include <cstdint>
#include <cmath>

class NoiseGenerator
{
public:
    // 0 = White, 1 = Pink, 2 = Brown
    void setNoiseType (int type) { noiseType = type; }

    void setSeed (uint32_t newSeed) { state = newSeed; }

    void prepare (double sampleRate)
    {
        // Brown noise one-pole LP coefficient (~20 Hz cutoff)
        float cutoffHz = 20.0f;
        brownCoeff = 1.0f - std::exp (-2.0f * 3.14159265f * cutoffHz / static_cast<float> (sampleRate));
    }

    float processSample()
    {
        float white = whiteNoise();

        switch (noiseType)
        {
            case 1: return pinkFilter (white);
            case 2: return brownFilter (white);
            default: return white;
        }
    }

    void renderBlock (float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

private:
    uint32_t state = 12345;
    int noiseType = 0;

    // ---- Pink noise: Paul Kellet pinking filter ----
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;

    float pinkFilter (float white)
    {
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;

        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;

        return pink * 0.11f; // normalize to approximately [-1, 1]
    }

    // ---- Brown noise: leaky integrator (one-pole LP on white noise) ----
    float brownState = 0.0f;
    float brownCoeff = 0.003f; // default for ~20Hz @ 44.1kHz

    float brownFilter (float white)
    {
        brownState += brownCoeff * (white - brownState);
        // Normalize: brown noise has much lower amplitude, scale up
        return std::clamp (brownState * 3.5f, -1.0f, 1.0f);
    }

    float whiteNoise()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) / static_cast<float> (INT32_MAX);
    }
};
