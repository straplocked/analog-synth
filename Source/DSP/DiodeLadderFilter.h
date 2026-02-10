#pragma once

#include <cmath>
#include <algorithm>

class DiodeLadderFilter
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
        // Input range 0-1, internal range 0-17 (diode ladder has higher feedback range)
        resonance = std::clamp (res, 0.0f, 1.0f) * 17.0f;
    }

    void setDrive (float driveAmount)
    {
        drive = std::max (1.0f, driveAmount);
    }

    float processSample (float input)
    {
        // Diode ladder filter: TPT topology with asymmetric clipping
        // Based on the Stilson/Smith model adapted for TPT

        // Apply drive
        float drivenInput = diodeClip (input * drive);

        // Resonance compensation
        drivenInput *= 1.0f / (1.0f + resonance * 0.15f);

        // Feedback from weighted sum of all stages (diode ladder characteristic)
        float feedback = resonance * (stage[3] * 0.5f + stage[2] * 0.3f + stage[1] * 0.15f + stage[0] * 0.05f);
        feedback = std::tanh (feedback);

        float u = drivenInput - feedback;

        // Four cascaded one-pole stages with asymmetric diode clipping
        for (int i = 0; i < 4; ++i)
        {
            float v = G * (diodeClip (u) - state[i]);
            float y = v + state[i];
            state[i] = y + v;
            stage[i] = y;
            u = y;
        }

        // DC blocker (5Hz high-pass) to prevent DC offset from asymmetric clipping
        float out = stage[3];
        float dcBlocked = out - dcState + 0.9999f * dcPrev;
        dcState = out;
        dcPrev = dcBlocked;

        return dcBlocked;
    }

    void reset()
    {
        for (int i = 0; i < 4; ++i)
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

    // Four filter stages
    float stage[4] = {};
    float state[4] = {};

    // DC blocker state
    float dcState = 0.0f;
    float dcPrev = 0.0f;

    // Asymmetric diode clipping — models forward-biased diode characteristic
    static float diodeClip (float x)
    {
        if (x > 0.0f)
            return std::tanh (x * 0.8f);  // softer positive clipping
        else
            return std::tanh (x * 1.2f);  // harder negative clipping
    }

    void updateCoefficients()
    {
        float wd = 2.0f * static_cast<float> (M_PI) * cutoff;
        float T = 1.0f / static_cast<float> (sampleRate);
        g = std::tan (wd * T / 2.0f);
        g = std::min (g, 10.0f);
        G = g / (1.0f + g);
    }
};
