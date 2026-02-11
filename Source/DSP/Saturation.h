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
