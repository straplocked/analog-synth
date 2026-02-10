#pragma once

#include <cmath>

namespace Saturation
{

inline float softClip (float x)
{
    return std::tanh (x);
}

inline float warmSaturate (float x, float drive)
{
    // Asymmetric saturation adds even harmonics for warmth
    float driven = x * drive;
    return std::tanh (driven) + 0.05f * driven * std::abs (driven);
}

inline float tapeStyle (float x, float drive)
{
    // Softer saturation curve, closer to tape compression
    float driven = x * drive;
    if (std::abs (driven) < 1.0f)
        return driven - (driven * driven * driven) / 3.0f;
    return (driven > 0.0f) ? 2.0f / 3.0f : -2.0f / 3.0f;
}

} // namespace Saturation
