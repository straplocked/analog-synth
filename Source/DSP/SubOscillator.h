#pragma once

#include <cmath>

class SubOscillator
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        phase = 0.0;
    }

    void setFrequency (float hz)
    {
        // Sub oscillator is always one octave below
        phaseIncrement = static_cast<double> (hz * 0.5f) / sampleRate;
    }

    float processSample()
    {
        // Simple triangle wave — at sub-octave frequencies, aliasing is minimal
        float output;
        if (phase < 0.5)
            output = static_cast<float> (4.0 * phase - 1.0);
        else
            output = static_cast<float> (3.0 - 4.0 * phase);

        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        return output;
    }

    void renderBlock (float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
};
