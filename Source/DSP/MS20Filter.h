#pragma once

#include <cmath>
#include <algorithm>
#include "FastMath.h"

// Korg MS-20 style Sallen-Key filter (2-pole)
// Asymmetric OTA/diode clipper saturation in feedback path
// Saturates rather than self-oscillates cleanly — distinctive gritty resonance
class MS20Filter
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
        // MS-20 has a wide resonance range — saturates aggressively
        resParam = std::clamp (res, 0.0f, 1.0f);
        resonance = resParam * 2.0f;
    }

    void setDrive (float driveAmount)
    {
        drive = std::max (1.0f, driveAmount);
    }

    float processSample (float input)
    {
        // Apply drive with saturation (prevents instability)
        float drivenInput = std::tanh (input * drive);

        // Resonance compensation
        drivenInput *= 1.0f / (1.0f + resParam * 0.8f);

        // Sallen-Key feedback with MS-20's characteristic asymmetric saturation
        float feedbackSignal = ms20Saturate (s2 * resonance);

        // Input with feedback
        float u = drivenInput - feedbackSignal;

        // Soft clip the input to prevent extreme feedback buildup
        u = std::tanh (u);

        // Two cascaded one-pole TPT integrators (Sallen-Key topology)
        float v1 = G * (u - s1);
        float y1 = v1 + s1;
        s1 = y1 + v1;

        float v2 = G * (y1 - s2);
        float y2 = v2 + s2;
        s2 = y2 + v2;

        // DC blocker (asymmetric saturation introduces DC offset)
        float dcBlocked = y2 - dcState + 0.9999f * dcPrev;
        dcState = y2;
        dcPrev = dcBlocked;

        return dcBlocked;
    }

    void reset()
    {
        s1 = 0.0f;
        s2 = 0.0f;
        dcState = 0.0f;
        dcPrev = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float cutoff = 8000.0f;
    float resonance = 0.0f;
    float drive = 1.0f;
    float resParam = 0.0f;

    // TPT coefficients
    float g = 0.0f;
    float G = 0.0f;

    // State variables
    float s1 = 0.0f;
    float s2 = 0.0f;

    // DC blocker state
    float dcState = 0.0f;
    float dcPrev = 0.0f;

    // MS-20 characteristic asymmetric OTA/diode saturation
    // Piecewise function modeling the MS-20's distinctive clipping
    static float ms20Saturate (float x)
    {
        if (x > 1.0f)
            return 1.0f + std::tanh (x - 1.0f);   // soft limit positive
        else if (x < -1.0f)
            return -1.0f - std::log1p (-(x + 1.0f)); // harder negative clip (diode)
        else
            return x;  // linear region
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
