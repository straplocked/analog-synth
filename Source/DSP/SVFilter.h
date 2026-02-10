#pragma once

#include <cmath>
#include <algorithm>
#include "FastMath.h"

// Sequential/Oberheim-style state-variable low-pass filter (2-pole TPT)
// Clean, musical resonance across the full range
class SVFilter
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
        // Input range 0-1, map to damping: R = 1/(2*Q)
        // At res=0: R=1 (Butterworth-ish), at res=1: R~0.04 (strong resonance)
        resParam = std::clamp (res, 0.0f, 1.0f);
        float q = 0.5f + resParam * 11.5f;  // Q from 0.5 to 12
        R = 1.0f / (2.0f * q);
    }

    void setDrive (float driveAmount)
    {
        drive = std::max (1.0f, driveAmount);
    }

    float processSample (float input)
    {
        // TPT state-variable filter — low-pass output
        float drivenInput = std::tanh (input * drive);

        // Resonance compensation with baseline attenuation — SVF has unity passband
        // gain unlike ladder filters whose cascaded tanh stages naturally attenuate
        drivenInput *= 0.33f / (1.0f + resParam * 0.05f);

        float hp = (drivenInput - (2.0f * R + g) * s1 - s2) / (1.0f + 2.0f * R * g + g * g);
        float bp = g * hp + s1;
        s1 = g * hp + bp;
        float lp = g * bp + s2;
        s2 = g * bp + lp;

        return lp;
    }

    void reset()
    {
        s1 = 0.0f;
        s2 = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float cutoff = 8000.0f;
    float drive = 1.0f;
    float resParam = 0.0f;

    // TPT coefficients
    float g = 0.0f;
    float R = 0.707f;  // damping

    // State variables
    float s1 = 0.0f;
    float s2 = 0.0f;

    void updateCoefficients()
    {
        float wd = 2.0f * static_cast<float> (M_PI) * cutoff;
        float T = 1.0f / static_cast<float> (sampleRate);
        g = FastMath::tan (wd * T / 2.0f);
        g = std::min (g, 10.0f);
    }
};
