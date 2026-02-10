#pragma once

#include <cmath>
#include <algorithm>

class LadderFilter
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
        // Input range 0-1, internal range 0-4 (self-oscillation near 4)
        resonance = std::clamp (res, 0.0f, 1.0f) * 4.0f;
    }

    void setDrive (float driveAmount)
    {
        drive = std::max (1.0f, driveAmount);
    }

    float processSample (float input)
    {
        // TPT (Topology-Preserving Transform) ladder filter
        // Based on the Välimäki/Huovilainen model with nonlinear feedback

        // Apply drive to input
        float drivenInput = std::tanh (input * drive);

        // Resonance volume compensation — real analog filters lose bass at high resonance
        drivenInput *= 1.0f / (1.0f + resonance * 0.25f);

        // Feedback with nonlinear saturation
        float feedback = resonance * std::tanh (stage[3]);

        // Input with feedback subtracted
        float u = drivenInput - feedback;

        // Four cascaded one-pole stages with saturation
        // Using proper TPT resolving factor: G = g / (1 + g)
        for (int i = 0; i < 4; ++i)
        {
            float v = G * (std::tanh (u) - state[i]);
            float y = v + state[i];
            state[i] = y + v;  // trapezoidal integration state update
            stage[i] = y;
            u = y;
        }

        return stage[3];
    }

    void reset()
    {
        for (int i = 0; i < 4; ++i)
        {
            stage[i] = 0.0f;
            state[i] = 0.0f;
        }
    }

private:
    double sampleRate = 44100.0;
    float cutoff = 8000.0f;
    float resonance = 0.0f;
    float drive = 1.0f;

    // TPT coefficients
    float g = 0.0f;  // tan(pi * fc / fs)
    float G = 0.0f;  // g / (1 + g) — resolved one-pole gain

    // Four filter stages
    float stage[4] = {};
    float state[4] = {};

    void updateCoefficients()
    {
        // g = tan(pi * cutoff / sampleRate)
        float wd = 2.0f * static_cast<float> (M_PI) * cutoff;
        float T = 1.0f / static_cast<float> (sampleRate);
        g = std::tan (wd * T / 2.0f);

        // Clamp g to prevent instability at extreme cutoff near Nyquist
        g = std::min (g, 10.0f);

        // Resolved gain — this is the critical stability factor
        G = g / (1.0f + g);
    }
};
