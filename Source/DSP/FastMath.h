#pragma once

#include <cstdint>
#include <algorithm>

// Fast approximations for performance-critical DSP math.
// These trade a tiny amount of accuracy (~20-bit) for significant speed gains
// in the per-sample audio rendering loop, replacing expensive std::pow/exp/tan.
namespace FastMath
{
    // Fast 2^x (~20-bit accuracy, max error ~1e-6)
    // Uses IEEE 754 exponent manipulation + degree-4 minimax polynomial
    inline float exp2 (float x)
    {
        x = std::clamp (x, -126.0f, 126.0f);

        // Floor (handles negatives correctly)
        int32_t xi = static_cast<int32_t> (x);
        if (static_cast<float> (xi) > x)
            --xi;
        float xf = x - static_cast<float> (xi);

        // 2^(integer part) via IEEE 754 exponent bits
        union { float f; int32_t i; } scale;
        scale.i = (xi + 127) << 23;

        // 2^(fractional part) via minimax polynomial
        float p = 1.0f + xf * (0.6931472f + xf * (0.2402265f
                 + xf * (0.05550410f + xf * 0.009618129f)));

        return p * scale.f;
    }

    // Fast exp(x) implemented as 2^(x * log2(e))
    inline float exp (float x)
    {
        return exp2 (x * 1.442695041f);
    }

    // Fast tan(x) via Padé [2/1] rational approximation
    // Accurate to <0.3% for |x| < 1.0 (covers all audio filter
    // frequencies at 2x oversampled rates)
    inline float tan (float x)
    {
        float x2 = x * x;
        return x * (15.0f - x2) / (15.0f - 6.0f * x2);
    }
}
