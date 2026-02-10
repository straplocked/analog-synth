#pragma once

#include <cmath>
#include <atomic>

class SmoothParam
{
public:
    void prepare (double sampleRate, float smoothingTimeMs = 5.0f)
    {
        coeff = 1.0f - std::exp (-1.0f / (static_cast<float> (sampleRate) * smoothingTimeMs * 0.001f));
    }

    void setTargetValue (float newTarget) { target = newTarget; }

    float getNextValue()
    {
        current += coeff * (target - current);
        return current;
    }

    float getCurrentValue() const { return current; }

    void reset (float value)
    {
        current = value;
        target = value;
    }

private:
    float current = 0.0f;
    float target = 0.0f;
    float coeff = 0.1f;
};
