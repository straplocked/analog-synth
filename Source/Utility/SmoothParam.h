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
