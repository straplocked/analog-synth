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
