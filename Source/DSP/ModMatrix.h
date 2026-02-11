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

#include <cstdint>
#include <algorithm>

// Modulation sources available in the mod matrix
enum class ModSource : int
{
    None = 0,
    FilterEnv,
    AmpEnv,
    Env3,
    Env4,
    LFO1,
    LFO2,
    LFO3,
    Velocity,
    NoteNumber,
    ModWheel,
    Aftertouch,
    PitchBend,
    Random,
    Macro1,
    Macro2,
    Macro3,
    Macro4,
    Chaos,
    NumSources
};

// Modulation destinations available in the mod matrix
enum class ModDest : int
{
    None = 0,
    Osc1Freq,
    Osc2Freq,
    Osc3Freq,
    Osc1PW,
    Osc2PW,
    Osc3PW,
    Osc1Level,
    Osc2Level,
    Osc3Level,
    SubLevel,
    NoiseLevel,
    LPFCutoff,
    LPFResonance,
    HPFCutoff,
    FilterDrive,
    FilterEnvAmt,
    AmpLevel,
    LFO1Rate,
    LFO2Rate,
    LFO3Rate,
    DistDrive,
    DistMix,
    ChorusRate,
    ChorusDepth,
    ChorusMix,
    DelayTime,
    DelayFeedback,
    DelayMix,
    ReverbSize,
    ReverbMix,
    NumDests
};

// Per-voice modulation source values, updated per-sample in renderOversampled()
struct VoiceModValues
{
    float filterEnv = 0.0f;
    float ampEnv = 0.0f;
    float env3 = 0.0f;
    float env4 = 0.0f;
    float lfo1 = 0.0f;
    float lfo2 = 0.0f;
    float lfo3 = 0.0f;
    float velocity = 0.0f;
    float noteNumber = 0.0f;   // normalized: (midiNote - 60) / 60
    float random = 0.0f;       // random value set at noteOn, held per-note
};

// Global modulation source values, updated per-block in processBlock()
struct GlobalModValues
{
    float modWheel = 0.0f;     // 0 to 1
    float aftertouch = 0.0f;   // 0 to 1
    float pitchBend = 0.0f;    // -1 to +1
    float macro1 = 0.0f;       // 0 to 1
    float macro2 = 0.0f;
    float macro3 = 0.0f;
    float macro4 = 0.0f;
    float chaos = 0.0f;        // -1 to +1
};

// A single routing slot in the mod matrix
struct ModSlot
{
    ModSource source = ModSource::None;
    ModDest dest = ModDest::None;
    float amount = 0.0f;

    bool isActive() const { return source != ModSource::None && dest != ModDest::None && amount != 0.0f; }
};

class ModMatrix
{
public:
    static constexpr int kNumSlots = 16;

    // Update slot configuration from APVTS parameter values.
    // Call once per block before rendering voices.
    void setSlot (int index, int sourceIndex, int destIndex, float amount)
    {
        if (index < 0 || index >= kNumSlots)
            return;

        slots[index].source = static_cast<ModSource> (std::clamp (sourceIndex, 0, static_cast<int> (ModSource::NumSources) - 1));
        slots[index].dest = static_cast<ModDest> (std::clamp (destIndex, 0, static_cast<int> (ModDest::NumDests) - 1));
        slots[index].amount = std::clamp (amount, -1.0f, 1.0f);
    }

    // Pre-compute the active destination bitmask. Call once per block after setSlot().
    void prepareDestinationMask()
    {
        activeDestMask = 0;
        for (int i = 0; i < kNumSlots; ++i)
        {
            if (slots[i].isActive())
                activeDestMask |= (uint64_t (1) << static_cast<int> (slots[i].dest));
        }
    }

    // O(1) check whether any routing targets a given destination
    bool hasModulationFor (ModDest dest) const
    {
        return (activeDestMask & (uint64_t (1) << static_cast<int> (dest))) != 0;
    }

    // Sum all active routings for a given destination.
    // Returns the total modulation amount (-N to +N, caller should scale/clamp).
    float computeModulation (ModDest dest, const VoiceModValues& voice, const GlobalModValues& global) const
    {
        float total = 0.0f;

        for (int i = 0; i < kNumSlots; ++i)
        {
            if (slots[i].dest != dest || ! slots[i].isActive())
                continue;

            float sourceVal = getSourceValue (slots[i].source, voice, global);
            total += sourceVal * slots[i].amount;
        }

        return total;
    }

    const ModSlot& getSlot (int index) const { return slots[index]; }

private:
    ModSlot slots[kNumSlots] = {};
    uint64_t activeDestMask = 0;

    static float getSourceValue (ModSource source, const VoiceModValues& voice, const GlobalModValues& global)
    {
        switch (source)
        {
            case ModSource::FilterEnv:   return voice.filterEnv;
            case ModSource::AmpEnv:      return voice.ampEnv;
            case ModSource::Env3:        return voice.env3;
            case ModSource::Env4:        return voice.env4;
            case ModSource::LFO1:        return voice.lfo1;
            case ModSource::LFO2:        return voice.lfo2;
            case ModSource::LFO3:        return voice.lfo3;
            case ModSource::Velocity:    return voice.velocity;
            case ModSource::NoteNumber:  return voice.noteNumber;
            case ModSource::ModWheel:    return global.modWheel;
            case ModSource::Aftertouch:  return global.aftertouch;
            case ModSource::PitchBend:   return global.pitchBend;
            case ModSource::Random:      return voice.random;
            case ModSource::Macro1:      return global.macro1;
            case ModSource::Macro2:      return global.macro2;
            case ModSource::Macro3:      return global.macro3;
            case ModSource::Macro4:      return global.macro4;
            case ModSource::Chaos:       return global.chaos;
            default:                     return 0.0f;
        }
    }
};
