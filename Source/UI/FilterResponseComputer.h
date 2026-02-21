/*
 * AnalogSynth - Virtual analog synthesizer plugin
 * Copyright (C) 2025 AnalogSynth contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cmath>
#include <array>

class FilterResponseComputer
{
public:
    // Evaluate biquad magnitude response in dB at normalised angular frequency w (0..pi)
    // coeffs: { b0, b1, b2, a0, a1, a2 } — 6-element array with a0=1 pre-inserted.
    // NOTE: This is NOT the raw JUCE IIR::Coefficients layout (which stores only 5 floats
    // without a0). Use the extractCoeffs helper in WaveformDisplay to convert.
    static float biquadMagnitudeDB (const float* coeffs, float w)
    {
        // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (a0 + a1*z^-1 + a2*z^-2)
        float cosw  = std::cos (w);
        float cos2w = std::cos (2.0f * w);
        float sinw  = std::sin (w);
        float sin2w = std::sin (2.0f * w);

        float b0 = coeffs[0], b1 = coeffs[1], b2 = coeffs[2];
        float a0 = coeffs[3], a1 = coeffs[4], a2 = coeffs[5];

        // Numerator: |B(e^jw)|^2
        float numReal = b0 + b1 * cosw + b2 * cos2w;
        float numImag = -(b1 * sinw + b2 * sin2w);
        float numMagSq = numReal * numReal + numImag * numImag;

        // Denominator: |A(e^jw)|^2
        float denReal = a0 + a1 * cosw + a2 * cos2w;
        float denImag = -(a1 * sinw + a2 * sin2w);
        float denMagSq = denReal * denReal + denImag * denImag;

        if (denMagSq < 1e-20f) return -100.0f;

        float magSq = numMagSq / denMagSq;
        if (magSq < 1e-20f) return -100.0f;

        return 10.0f * std::log10 (magSq);
    }

    // Analytic TPT ladder filter magnitude response in dB.
    // This is the linearized model (ignores tanh nonlinearity at high resonance).
    // Accurate at low-to-moderate resonance; deviates at high resonance where
    // tanh saturation compresses the feedback path.
    static float ladderMagnitudeDB (float cutoffHz, float resonance01, float sr, bool twoPole, float freqHz)
    {
        float pi = 3.14159265358979323846f;

        // g = tan(pi * fc / sr) — same as LadderFilter::updateCoefficients()
        float g = std::tan (pi * cutoffHz / sr);
        g = std::min (g, 10.0f);

        float G = g / (1.0f + g);
        float res = resonance01 * 4.0f;  // internal range 0-4

        // Evaluate at the query frequency
        float w = 2.0f * pi * freqHz / sr;

        // One-pole TPT transfer function: H1(z) = G / (1 - (1-G)*z^-1)
        // |H1(e^jw)|^2 at angular frequency w
        float cosw = std::cos (w);
        float oneMinusG = 1.0f - G;

        // |H1|^2 = G^2 / (1 - 2*(1-G)*cos(w) + (1-G)^2)
        float h1denSq = 1.0f - 2.0f * oneMinusG * cosw + oneMinusG * oneMinusG;
        if (h1denSq < 1e-20f) h1denSq = 1e-20f;
        float h1magSq = (G * G) / h1denSq;

        float cascadeMagSq;
        if (twoPole)
            cascadeMagSq = h1magSq * h1magSq;  // 2 stages
        else
            cascadeMagSq = h1magSq * h1magSq * h1magSq * h1magSq;  // 4 stages

        // With resonance feedback: H(z) = Hcascade(z) / (1 + res * Hcascade(z))
        // |H|^2 = |Hc|^2 / |1 + res * Hc|^2
        // For the magnitude approximation, we use the fact that the cascade
        // output is real-valued at DC and Nyquist, and approximate using magnitude only
        float cascadeMag = std::sqrt (cascadeMagSq);
        float resDenomSq = 1.0f + 2.0f * res * cascadeMag + res * res * cascadeMagSq;
        if (resDenomSq < 1e-20f) resDenomSq = 1e-20f;

        float totalMagSq = cascadeMagSq / resDenomSq;
        if (totalMagSq < 1e-20f) return -100.0f;

        return 10.0f * std::log10 (totalMagSq);
    }

    // Analytic 2-pole (12 dB/oct) state-variable HPF magnitude response in dB.
    // Matches StateVariableHPF with Q = 0.707 (Butterworth).
    static float hpfMagnitudeDB (float cutoffHz, float sr, float freqHz)
    {
        float pi = 3.14159265358979323846f;
        float g = std::tan (pi * cutoffHz / sr);
        float R = 0.707f;

        // TPT SVF high-pass transfer function:
        // H_hp(s) = s^2 / (s^2 + 2Rs + 1)  in the analog prototype
        // In the digital domain via bilinear transform with g = tan(pi*fc/fs):
        // Using the frequency warped variable, evaluate at the query frequency
        float w = 2.0f * pi * freqHz / sr;
        float cosw = std::cos (w);
        float sinw = std::sin (w);

        // The SVF HP output: hp = (x - (2R+g)*s1 - s2) / (1 + 2Rg + g^2)
        // Transfer function H(z) for TPT SVF HP:
        // Numerator: 1 - 2*cos(w) + cos(2w), etc.
        // Easier to evaluate directly using the analog-to-digital mapping:
        // At frequency f, the bilinear transform gives s = j*tan(pi*f/sr)
        float s_mag = std::tan (pi * freqHz / sr);
        // Clamp to prevent infinity near Nyquist
        s_mag = std::min (s_mag, 100.0f);

        // H_hp(s) = s^2 / (s^2 + 2Rs + 1)  where s is normalized by cutoff
        // The TPT mapping: s_normalized = s_mag / g
        float sn = s_mag / std::max (g, 1e-10f);

        // |H_hp(j*sn)|^2 = sn^4 / ((1-sn^2)^2 + (2R*sn)^2)
        float sn2 = sn * sn;
        float sn4 = sn2 * sn2;
        float denom = (1.0f - sn2) * (1.0f - sn2) + (2.0f * R * sn) * (2.0f * R * sn);
        if (denom < 1e-20f) denom = 1e-20f;

        float magSq = sn4 / denom;
        if (magSq < 1e-20f) return -100.0f;

        return 10.0f * std::log10 (magSq);
    }

    // Combined 3-band parametric EQ response in dB.
    // Each coeffs array: { b0, b1, b2, a0, a1, a2 }
    static float eqMagnitudeDB (const float* lowCoeffs, const float* midCoeffs,
                                 const float* highCoeffs, float freqHz, float sr)
    {
        float pi = 3.14159265358979323846f;
        float w = 2.0f * pi * freqHz / sr;

        float lowDb  = biquadMagnitudeDB (lowCoeffs, w);
        float midDb  = biquadMagnitudeDB (midCoeffs, w);
        float highDb = biquadMagnitudeDB (highCoeffs, w);

        return lowDb + midDb + highDb;
    }
};
