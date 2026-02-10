#pragma once

#include <cmath>
#include <algorithm>

class SubOscillator
{
public:
    enum class Waveform { Sine, Triangle, Square, Pulse };

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        phase = 0.0;
        phaseIncrement = 0.0;
    }

    void setFrequency (float hz)
    {
        baseHz = hz;
        updatePhaseIncrement();
    }

    void setWaveform (Waveform w) { waveform = w; }

    void setOctave (int oct)
    {
        octaveOffset = std::clamp (oct, -2, -1);
        updatePhaseIncrement();
    }

    void setPulseWidth (float pw) { pulseWidth = std::clamp (pw, 0.05f, 0.95f); }

    void setPhase (double newPhase) { phase = std::fmod (std::abs (newPhase), 1.0); }

    float processSample()
    {
        float output = 0.0f;
        double dt = phaseIncrement;

        switch (waveform)
        {
            case Waveform::Sine:
                output = static_cast<float> (std::sin (2.0 * M_PI * phase));
                break;

            case Waveform::Triangle:
                if (phase < 0.5)
                    output = static_cast<float> (4.0 * phase - 1.0);
                else
                    output = static_cast<float> (3.0 - 4.0 * phase);
                break;

            case Waveform::Square:
            {
                double saw1 = 2.0 * phase - 1.0;
                double blep1 = polyBLEP (phase, dt);
                double shiftedPhase = phase + 0.5;
                if (shiftedPhase >= 1.0)
                    shiftedPhase -= 1.0;
                double saw2 = 2.0 * shiftedPhase - 1.0;
                double blep2 = polyBLEP (shiftedPhase, dt);
                output = static_cast<float> ((saw1 - blep1) - (saw2 - blep2));
                break;
            }

            case Waveform::Pulse:
            {
                double pw = static_cast<double> (pulseWidth);
                double saw1 = 2.0 * phase - 1.0;
                double blep1 = polyBLEP (phase, dt);
                double shiftedPhase = phase + (1.0 - pw);
                if (shiftedPhase >= 1.0)
                    shiftedPhase -= 1.0;
                double saw2 = 2.0 * shiftedPhase - 1.0;
                double blep2 = polyBLEP (shiftedPhase, dt);
                output = static_cast<float> ((saw1 - blep1) - (saw2 - blep2));
                break;
            }
        }

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
    float baseHz = 220.0f;
    Waveform waveform = Waveform::Triangle;
    int octaveOffset = -1;
    float pulseWidth = 0.5f;

    void updatePhaseIncrement()
    {
        double freq = static_cast<double> (baseHz) * std::pow (2.0, static_cast<double> (octaveOffset));
        phaseIncrement = std::max (freq, 1.0) / sampleRate;
    }

    static double polyBLEP (double t, double dt)
    {
        if (dt <= 0.0)
            return 0.0;
        if (t < dt)
        {
            double x = t / dt;
            return x + x - x * x - 1.0;
        }
        else if (t > 1.0 - dt)
        {
            double x = (t - 1.0) / dt;
            return x * x + x + x + 1.0;
        }
        return 0.0;
    }
};
