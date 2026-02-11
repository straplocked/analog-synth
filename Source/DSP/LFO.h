#pragma once

#include <cmath>
#include <cstdint>

class LFO
{
public:
    enum class Shape { Triangle, Saw, Square, Random, Sine };

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        phase = voicePhaseOffset;
        randomValue = 0.0f;
        prevPhase = voicePhaseOffset;
        slewState = 0.0f;
        // 2ms slew time: smooths clicks without removing punch
        slewCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (newSampleRate) * 0.002f));
    }

    void setShape (int shapeIndex) { shape = static_cast<Shape> (shapeIndex); }
    void setRate (float hz) { rate = hz; }
    void setKeySync (bool enabled) { keySync = enabled; }

    void resetPhase() { phase = voicePhaseOffset; }

    void setVoicePhaseOffset (double offset)
    {
        voicePhaseOffset = std::fmod (std::abs (offset), 1.0);
    }

    // Returns bipolar output [-1, +1]
    float processSample()
    {
        float output = 0.0f;
        bool needsSlew = false;

        switch (shape)
        {
            case Shape::Triangle:
                if (phase < 0.5)
                    output = static_cast<float> (4.0 * phase - 1.0);
                else
                    output = static_cast<float> (3.0 - 4.0 * phase);
                break;

            case Shape::Saw:
                // Ramp down
                output = static_cast<float> (1.0 - 2.0 * phase);
                break;

            case Shape::Square:
                output = (phase < 0.5) ? 1.0f : -1.0f;
                needsSlew = true;
                break;

            case Shape::Random:
            {
                // Sample-and-hold: new random value at each cycle start
                if (phase < prevPhase) // wrapped around
                {
                    // Simple xorshift for random
                    randomState ^= randomState << 13;
                    randomState ^= randomState >> 17;
                    randomState ^= randomState << 5;
                    randomValue = static_cast<float> (static_cast<int32_t> (randomState))
                                / static_cast<float> (INT32_MAX);
                }
                output = randomValue;
                needsSlew = true;
                break;
            }

            case Shape::Sine:
                output = static_cast<float> (std::sin (phase * 2.0 * M_PI));
                break;
        }

        prevPhase = phase;
        phase += rate / sampleRate;
        if (phase >= 1.0)
            phase -= 1.0;

        // Apply one-pole slew to discontinuous waveforms (analog capacitor slew)
        if (needsSlew)
        {
            slewState += slewCoeff * (output - slewState);
            return slewState;
        }

        return output;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double prevPhase = 0.0;
    float rate = 2.0f;
    Shape shape = Shape::Triangle;
    bool keySync = false;

    // For S&H random mode
    uint32_t randomState = 54321;
    float randomValue = 0.0f;

    // Per-voice phase offset for natural modulation spread
    double voicePhaseOffset = 0.0;

    // Slew rate limiter for Square/S&H (analog capacitor slew)
    float slewState = 0.0f;
    float slewCoeff = 0.0f;
};
