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
    // Read FX order parameter and decode permutation
    int orderIndex = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::fxOrder)->load());
    processingOrder = FXOrderUtils::permutationFromIndex (orderIndex);

    // Let each effect read its own parameters
    for (auto& fx : effects)
        fx->updateParameters (apvts);
}
