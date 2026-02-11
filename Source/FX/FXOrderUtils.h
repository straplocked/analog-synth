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

#include <array>
#include <algorithm>

namespace FXOrderUtils
{

// 6 effects: 0=Distortion, 1=Compressor, 2=EQ, 3=Chorus, 4=Delay, 5=Reverb
static constexpr int kNumEffects = 6;

// 6! = 720 total permutations
static constexpr int kMaxPermutations = 720;

// Convert a permutation index (0..719) to a permutation of {0,1,2,3,4,5}
// using factoradic (Lehmer code) decomposition.
inline std::array<int, kNumEffects> permutationFromIndex (int index)
{
    index = std::max (0, std::min (kMaxPermutations - 1, index));

    std::array<int, kNumEffects> available = { 0, 1, 2, 3, 4, 5 };
    std::array<int, kNumEffects> result {};
    int n = kNumEffects;

    for (int i = 0; i < kNumEffects; ++i)
    {
        int factorial = 1;
        for (int f = 1; f < n; ++f)
            factorial *= f;

        int digit = index / factorial;
        index %= factorial;

        result[static_cast<std::size_t> (i)] = available[static_cast<std::size_t> (digit)];

        // Remove used element by shifting
        for (int j = digit; j < n - 1; ++j)
            available[static_cast<std::size_t> (j)] = available[static_cast<std::size_t> (j + 1)];
        --n;
    }

    return result;
}

// Convert a permutation of {0,1,2,3,4,5} back to an index (0..719)
inline int indexFromPermutation (const std::array<int, kNumEffects>& perm)
{
    std::array<int, kNumEffects> available = { 0, 1, 2, 3, 4, 5 };
    int index = 0;
    int n = kNumEffects;

    for (int i = 0; i < kNumEffects; ++i)
    {
        int factorial = 1;
        for (int f = 1; f < n; ++f)
            factorial *= f;

        // Find position of perm[i] in available
        int pos = 0;
        for (int j = 0; j < n; ++j)
        {
            if (available[static_cast<std::size_t> (j)] == perm[static_cast<std::size_t> (i)])
            {
                pos = j;
                break;
            }
        }

        index += pos * factorial;

        // Remove used element
        for (int j = pos; j < n - 1; ++j)
            available[static_cast<std::size_t> (j)] = available[static_cast<std::size_t> (j + 1)];
        --n;
    }

    return index;
}

} // namespace FXOrderUtils
