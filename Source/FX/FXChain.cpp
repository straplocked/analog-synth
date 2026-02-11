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

#include "FXChain.h"
#include "../ParameterIDs.h"

void FXChain::updateParameters (juce::AudioProcessorValueTreeState& apvts)
{
    // Read slot-based ordering
    std::array<int, kNumEffects> newOrder;
    for (int i = 0; i < kNumEffects; ++i)
        newOrder[static_cast<std::size_t> (i)] = static_cast<int> (
            apvts.getRawParameterValue (ParameterIDs::fxOrderSlotIDs[i])->load());

    // Migration: if all slots are at identity defaults and old factoradic fxOrder != 0,
    // decode the legacy 6-effect order and append new effects at the end
    bool allDefaults = true;
    for (int i = 0; i < kNumEffects; ++i)
    {
        if (newOrder[static_cast<std::size_t> (i)] != i)
        {
            allDefaults = false;
            break;
        }
    }

    if (allDefaults)
    {
        int oldOrder = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::fxOrder)->load());
        if (oldOrder != 0)
        {
            auto legacy = FXOrderUtils::permutationFromIndex (oldOrder);
            for (int i = 0; i < FXOrderUtils::kNumEffects; ++i)
                newOrder[static_cast<std::size_t> (i)] = legacy[static_cast<std::size_t> (i)];
            newOrder[6] = kPhaser;
            newOrder[7] = kFlanger;
            newOrder[8] = kHyper;
        }
    }

    // Validate: each effect index must appear exactly once
    std::array<bool, kNumEffects> seen {};
    bool valid = true;
    for (int i = 0; i < kNumEffects; ++i)
    {
        int idx = newOrder[static_cast<std::size_t> (i)];
        if (idx < 0 || idx >= kNumEffects || seen[static_cast<std::size_t> (idx)])
        {
            valid = false;
            break;
        }
        seen[static_cast<std::size_t> (idx)] = true;
    }

    if (valid)
        processingOrder = newOrder;
    // else keep previous valid order

    // Let each effect read its own parameters
    for (auto& fx : effects)
        fx->updateParameters (apvts);
}
