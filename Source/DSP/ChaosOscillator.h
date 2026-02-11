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
#include <algorithm>

// Lorenz attractor running at a slow rate, outputting a continuously
// evolving chaotic signal in the range [-1, +1]. Used as a global
// modulation source routed through the mod matrix.
class ChaosOscillator
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        // Initial conditions (off the attractor's fixed point to ensure motion)
        x = 0.1;
        y = 0.0;
        z = 0.0;
    }

    // rate: controls simulation speed (0.01 to 10 Hz conceptually)
    // amount: output scaling (0 to 1)
    void setRate (float rateHz) { rate = rateHz; }
    void setAmount (float amt) { amount = amt; }

    // Process one sample. Returns a value in [-1, +1].
    float processSample()
    {
        // Lorenz system parameters (classic chaotic regime)
        constexpr double sigma = 10.0;
        constexpr double rho = 28.0;
        constexpr double beta = 8.0 / 3.0;

        // Scale the integration step by rate.
        // At rate=1, we step slowly enough that the output evolves audibly over ~1 second.
        // The base dt is chosen so the attractor runs at a musically useful pace.
        double dt = static_cast<double> (rate) * 0.0002 / (sr / 44100.0);

        // Euler integration of the Lorenz system
        double dx = sigma * (y - x);
        double dy = x * (rho - z) - y;
        double dz = x * y - beta * z;

        x += dx * dt;
        y += dy * dt;
        z += dz * dt;

        // The Lorenz x variable ranges roughly [-20, +20].
        // Normalize to [-1, +1] and scale by amount.
        float output = static_cast<float> (std::clamp (x / 20.0, -1.0, 1.0)) * amount;
        return output;
    }

private:
    double sr = 44100.0;
    float rate = 1.0f;
    float amount = 0.5f;

    // Lorenz state variables
    double x = 0.1;
    double y = 0.0;
    double z = 0.0;
};
