#pragma once

#include <cmath>
#include <cstdint>

class AnalogDrift
{
public:
    void prepare (double newSampleRate, uint32_t voiceIndex)
    {
        sampleRate = newSampleRate;

        // Each voice gets unique phase offsets based on voice index
        // Use irrational frequency ratios for non-repeating drift patterns
        float seed = static_cast<float> (voiceIndex) * 1.618033988f; // golden ratio

        phase1 = seed;
        phase2 = seed * 2.414213562f; // 1 + sqrt(2)
        phase3 = seed * 3.302775638f; // 1 + e

        // Per-voice fixed offsets for component tolerance variation (-1 to +1)
        filterOffset    = (static_cast<float> (voiceIndex * 7  % 13) / 13.0f - 0.5f) * 2.0f;
        envTimeOffset   = (static_cast<float> (voiceIndex * 11 % 17) / 17.0f - 0.5f) * 2.0f;
        resonanceOffset = (static_cast<float> (voiceIndex * 13 % 19) / 19.0f - 0.5f) * 2.0f;
        driveOffset     = (static_cast<float> (voiceIndex * 17 % 29) / 29.0f - 0.5f) * 2.0f;
        lfoRateOffset   = (static_cast<float> (voiceIndex * 5  % 23) / 23.0f - 0.5f) * 2.0f;
        dcOffsetValue   = (static_cast<float> (voiceIndex * 3  % 11) / 11.0f - 0.5f) * 2.0f * 0.005f;
    }

    void setAmount (float newAmount) { amount = newAmount; }

    // Advance drift phases by numSamples (call once per block)
    void advance (int numSamples)
    {
        double n = static_cast<double> (numSamples);
        double inc1 = 0.05  / sampleRate * 2.0 * M_PI * n;
        double inc2 = 0.11  / sampleRate * 2.0 * M_PI * n;
        double inc3 = 0.077 / sampleRate * 2.0 * M_PI * n;

        phase1 += static_cast<float> (inc1);
        phase2 += static_cast<float> (inc2);
        phase3 += static_cast<float> (inc3);

        // Keep phases bounded
        constexpr float phaseBound = 100.0f * static_cast<float> (M_PI);
        if (phase1 > phaseBound) phase1 -= phaseBound;
        if (phase2 > phaseBound) phase2 -= phaseBound;
        if (phase3 > phaseBound) phase3 -= phaseBound;
    }

    // Get current pitch drift in cents (read-only, call after advance)
    float getPitchDriftCents() const
    {
        float drift = std::sin (phase1) * 0.5f
                    + std::sin (phase2) * 0.3f
                    + std::sin (phase3) * 0.2f;
        return drift * amount * 3.0f;
    }

    // Legacy per-sample version (kept for compatibility)
    float processSample()
    {
        advance (1);
        return getPitchDriftCents();
    }

    // Per-voice filter cutoff variation factor (multiply with base cutoff)
    float getFilterOffsetFactor() const
    {
        // amount 0-1 maps to ±5% of cutoff
        return 1.0f + filterOffset * amount * 0.05f;
    }

    // Per-voice envelope time variation factor
    float getEnvTimeOffsetFactor() const
    {
        return 1.0f + envTimeOffset * amount * 0.05f;
    }

    // Per-voice resonance variation factor (±4%)
    float getResonanceOffsetFactor() const
    {
        return 1.0f + resonanceOffset * amount * 0.04f;
    }

    // Per-voice filter drive variation factor (±3%)
    float getDriveOffsetFactor() const
    {
        return 1.0f + driveOffset * amount * 0.03f;
    }

    // Per-voice LFO rate variation factor (±5%)
    float getLFORateOffsetFactor() const
    {
        return 1.0f + lfoRateOffset * amount * 0.05f;
    }

    // Per-voice DC offset for even harmonics
    float getDCOffset() const
    {
        return dcOffsetValue * amount;
    }

private:
    double sampleRate = 44100.0;
    float amount = 0.2f;

    float phase1 = 0.0f;
    float phase2 = 0.0f;
    float phase3 = 0.0f;

    float filterOffset = 0.0f;
    float envTimeOffset = 0.0f;
    float resonanceOffset = 0.0f;
    float driveOffset = 0.0f;
    float lfoRateOffset = 0.0f;
    float dcOffsetValue = 0.0f;
};
