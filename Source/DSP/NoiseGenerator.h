#pragma once

#include <cstdint>

class NoiseGenerator
{
public:
    void setSeed (uint32_t newSeed) { state = newSeed; }

    float processSample()
    {
        // xorshift32 — fast, decent quality white noise
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;

        // Convert to float in range [-1, 1]
        return static_cast<float> (static_cast<int32_t> (state)) / static_cast<float> (INT32_MAX);
    }

    void renderBlock (float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

private:
    uint32_t state = 12345;
};
