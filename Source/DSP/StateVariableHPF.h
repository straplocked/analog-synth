#pragma once

#include <cmath>
#include <algorithm>

class StateVariableHPF
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

    float processSample (float input)
    {
        // TPT state-variable filter — high-pass output
        // 2-pole (12dB/oct) with fixed Q ~ 0.707 (Butterworth)
        float hp = (input - (2.0f * R + g) * s1 - s2) / (1.0f + 2.0f * R * g + g * g);
        float bp = g * hp + s1;
        s1 = g * hp + bp;
        float lp = g * bp + s2;
        s2 = g * bp + lp;

        return hp;
    }

    void reset()
    {
        s1 = 0.0f;
        s2 = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float cutoff = 20.0f;

    // TPT coefficients
    float g = 0.0f;
    float R = 0.707f; // damping (Butterworth Q)

    // State variables
    float s1 = 0.0f;
    float s2 = 0.0f;

    void updateCoefficients()
    {
        float wd = 2.0f * static_cast<float> (M_PI) * cutoff;
        float T = 1.0f / static_cast<float> (sampleRate);
        g = std::tan (wd * T / 2.0f);
    }
};
