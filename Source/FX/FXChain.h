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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <array>

#include "FXBase.h"
#include "FXOrderUtils.h"
#include "DistortionEffect.h"
#include "CompressorEffect.h"
#include "ParametricEQEffect.h"
#include "ChorusEffect.h"
#include "DelayEffect.h"
#include "ReverbEffect.h"

class FXChain
{
public:
    static constexpr int kNumEffects = FXOrderUtils::kNumEffects;

    // Effect index constants
    static constexpr int kDistortion  = 0;
    static constexpr int kCompressor  = 1;
    static constexpr int kEQ          = 2;
    static constexpr int kChorus      = 3;
    static constexpr int kDelay       = 4;
    static constexpr int kReverb      = 5;

    FXChain()
    {
        effects[kDistortion]  = std::make_unique<DistortionEffect>();
        effects[kCompressor]  = std::make_unique<CompressorEffect>();
        effects[kEQ]          = std::make_unique<ParametricEQEffect>();
        effects[kChorus]      = std::make_unique<ChorusEffect>();
        effects[kDelay]       = std::make_unique<DelayEffect>();
        effects[kReverb]      = std::make_unique<ReverbEffect>();

        // Default order: Dist, Comp, EQ, Chorus, Delay, Reverb
        for (int i = 0; i < kNumEffects; ++i)
            processingOrder[static_cast<std::size_t> (i)] = i;
    }

    void prepare (double sampleRate, int samplesPerBlock)
    {
        for (auto& fx : effects)
            fx->prepare (sampleRate, samplesPerBlock);
    }

    void setBpm (double newBpm)
    {
        // Forward BPM to the delay effect
        static_cast<DelayEffect*> (effects[kDelay].get())->setBpm (newBpm);
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts);

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        // Process effects in the current order
        for (int i = 0; i < kNumEffects; ++i)
        {
            int idx = processingOrder[static_cast<std::size_t> (i)];
            effects[static_cast<std::size_t> (idx)]->processBlock (buffer);
        }

        // NaN safety: if any FX produced bad output, clear and reset
        bool hasNaN = false;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (std::isnan (data[i]) || std::isinf (data[i]))
                {
                    hasNaN = true;
                    data[i] = 0.0f;
                }
            }
        }
        if (hasNaN)
        {
            for (auto& fx : effects)
                fx->reset();
        }
    }

    FXBase* getEffect (int index)
    {
        if (index >= 0 && index < kNumEffects)
            return effects[static_cast<std::size_t> (index)].get();
        return nullptr;
    }

    const std::array<int, kNumEffects>& getProcessingOrder() const { return processingOrder; }

private:
    std::array<std::unique_ptr<FXBase>, kNumEffects> effects;
    std::array<int, kNumEffects> processingOrder;
};
