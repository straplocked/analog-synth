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

namespace MathHelpers
{

inline float noteToFrequency (float midiNote, float tuningA4 = 440.0f)
{
    return tuningA4 * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
}

inline float frequencyToNote (float freqHz, float tuningA4 = 440.0f)
{
    return 69.0f + 12.0f * std::log2 (freqHz / tuningA4);
}

inline float dbToGain (float dB)
{
    return std::pow (10.0f, dB / 20.0f);
}

inline float gainToDb (float gain)
{
    return 20.0f * std::log10 (std::max (gain, 1e-10f));
}

inline float centsToRatio (float cents)
{
    return std::pow (2.0f, cents / 1200.0f);
}

} // namespace MathHelpers
