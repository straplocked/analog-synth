#pragma once

struct PolyModAmounts
{
    // Filter Envelope sources
    float fenvToOsc1Freq  = 0.0f;
    float fenvToOsc1Shape = 0.0f;
    float fenvToOsc1PW    = 0.0f;
    float fenvToLPF       = 0.0f;
    float fenvToHPF       = 0.0f;

    // Osc2 sources (audio-rate modulation)
    float osc2ToOsc1Freq  = 0.0f;
    float osc2ToOsc1Shape = 0.0f;
    float osc2ToOsc1PW    = 0.0f;
    float osc2ToLPF       = 0.0f;
    float osc2ToHPF       = 0.0f;

    bool isActive() const
    {
        return fenvToOsc1Freq  != 0.0f || fenvToOsc1Shape != 0.0f || fenvToOsc1PW    != 0.0f
            || fenvToLPF       != 0.0f || fenvToHPF       != 0.0f
            || osc2ToOsc1Freq  != 0.0f || osc2ToOsc1Shape != 0.0f || osc2ToOsc1PW    != 0.0f
            || osc2ToLPF       != 0.0f || osc2ToHPF       != 0.0f;
    }
};

struct LFOAmounts
{
    float toOscFreq  = 0.0f;
    float toOscShape = 0.0f;
    float toFilter   = 0.0f;
    float toAmp      = 0.0f;

    bool isActive() const
    {
        return toOscFreq != 0.0f || toOscShape != 0.0f || toFilter != 0.0f || toAmp != 0.0f;
    }
};
