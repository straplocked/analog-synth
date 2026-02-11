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

#include "catch_amalgamated.hpp"
#include "DSP/StateVariableHPF.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("StateVariableHPF basic operation", "[dsp]")
{
    StateVariableHPF hpf;
    hpf.prepare (kSampleRate);
    hpf.setCutoff (500.0f);

    SECTION ("Silence in -> silence out")
    {
        for (int i = 0; i < 1000; ++i)
            REQUIRE (hpf.processSample (0.0f) == Catch::Approx (0.0f).margin (1e-10));
    }

    SECTION ("Finite output for normal input")
    {
        for (int i = 0; i < 10000; ++i)
        {
            float input = std::sin (2.0f * static_cast<float> (M_PI) * 1000.0f * static_cast<float> (i) / static_cast<float> (kSampleRate));
            float output = hpf.processSample (input);
            REQUIRE (std::isfinite (output));
        }
    }
}

TEST_CASE ("StateVariableHPF high-pass behavior", "[dsp]")
{
    StateVariableHPF hpf;
    hpf.prepare (kSampleRate);
    hpf.setCutoff (1000.0f);

    SECTION ("Passes high frequencies, attenuates low frequencies")
    {
        auto measureAmplitude = [&] (float freq) {
            hpf.reset();
            hpf.setCutoff (1000.0f);
            float maxOut = 0.0f;
            for (int i = 0; i < 20000; ++i)
            {
                float input = std::sin (2.0f * static_cast<float> (M_PI) * freq * static_cast<float> (i) / static_cast<float> (kSampleRate));
                float output = hpf.processSample (input);
                if (i > 10000)
                    maxOut = std::max (maxOut, std::abs (output));
            }
            return maxOut;
        };

        float lowAmp = measureAmplitude (100.0f);
        float highAmp = measureAmplitude (5000.0f);

        // High frequency should be louder than low frequency
        REQUIRE (highAmp > lowAmp * 2.0f);
    }
}

TEST_CASE ("StateVariableHPF blocks DC", "[dsp]")
{
    StateVariableHPF hpf;
    hpf.prepare (kSampleRate);
    hpf.setCutoff (20.0f);

    SECTION ("DC signal is attenuated over time")
    {
        float output = 0.0f;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
            output = hpf.processSample (1.0f);

        // After 1 second with DC input, HPF output should be near zero
        REQUIRE (std::abs (output) < 0.01f);
    }
}

TEST_CASE ("StateVariableHPF reset", "[dsp]")
{
    StateVariableHPF hpf;
    hpf.prepare (kSampleRate);
    hpf.setCutoff (500.0f);

    for (int i = 0; i < 1000; ++i)
        hpf.processSample (1.0f);

    hpf.reset();

    SECTION ("Output zero after reset")
    {
        REQUIRE (hpf.processSample (0.0f) == Catch::Approx (0.0f).margin (1e-6));
    }
}
