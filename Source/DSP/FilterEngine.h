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

#include "LadderFilter.h"
#include "DiodeLadderFilter.h"
#include "SVFilter.h"
#include "AcidFilter.h"
#include "MS20Filter.h"

// Unified filter routing engine
// Manages all filter types with slope switching (12dB/24dB)
// 4-pole filters (Ladder, Diode): use internal stage tapping for 12dB mode
// 2-pole filters (SVF, Acid, MS20): cascade two instances for 24dB mode
//
// IMPORTANT: setCutoff is called per-sample in the render loop, so it only
// updates the active filter to avoid expensive trig calls on inactive filters.
// setResonance/setDrive are called per-block and update all filters for
// click-free type switching.
class FilterEngine
{
public:
    enum class Type { Ladder = 0, Diode, Sequential, Acid, MS20 };
    enum class Slope { TwelveDB = 0, TwentyFourDB };

    void prepare (double sampleRate)
    {
        ladder.prepare (sampleRate);
        diode.prepare (sampleRate);
        svf1.prepare (sampleRate);
        svf2.prepare (sampleRate);
        acid1.prepare (sampleRate);
        acid2.prepare (sampleRate);
        ms201.prepare (sampleRate);
        ms202.prepare (sampleRate);
    }

    void setType (int typeIndex)
    {
        activeType = static_cast<Type> (std::clamp (typeIndex, 0, 4));
        updateSlopeMode();
    }

    void setSlope (int slopeIndex)
    {
        activeSlope = static_cast<Slope> (std::clamp (slopeIndex, 0, 1));
        updateSlopeMode();
    }

    // Called per-sample — only update active filter(s)
    void setCutoff (float hz)
    {
        switch (activeType)
        {
            case Type::Ladder:
                ladder.setCutoff (hz);
                break;
            case Type::Diode:
                diode.setCutoff (hz);
                break;
            case Type::Sequential:
                svf1.setCutoff (hz);
                if (activeSlope == Slope::TwentyFourDB)
                    svf2.setCutoff (hz);
                break;
            case Type::Acid:
                acid1.setCutoff (hz);
                if (activeSlope == Slope::TwentyFourDB)
                    acid2.setCutoff (hz);
                break;
            case Type::MS20:
                ms201.setCutoff (hz);
                if (activeSlope == Slope::TwentyFourDB)
                    ms202.setCutoff (hz);
                break;
        }
    }

    // Called per-block — update all for click-free switching
    void setResonance (float res)
    {
        ladder.setResonance (res);
        diode.setResonance (res);
        svf1.setResonance (res);
        svf2.setResonance (res);
        acid1.setResonance (res);
        acid2.setResonance (res);
        ms201.setResonance (res);
        ms202.setResonance (res);
    }

    // Called per-block — update all for click-free switching
    void setDrive (float driveAmount)
    {
        ladder.setDrive (driveAmount);
        diode.setDrive (driveAmount);
        svf1.setDrive (driveAmount);
        svf2.setDrive (driveAmount);
        acid1.setDrive (driveAmount);
        acid2.setDrive (driveAmount);
        ms201.setDrive (driveAmount);
        ms202.setDrive (driveAmount);
    }

    float processSample (float input)
    {
        switch (activeType)
        {
            case Type::Ladder:
                return ladder.processSample (input);

            case Type::Diode:
                return diode.processSample (input);

            case Type::Sequential:
            {
                float out = svf1.processSample (input);
                if (activeSlope == Slope::TwentyFourDB)
                    out = svf2.processSample (out);
                return out * 1.25f;
            }

            case Type::Acid:
            {
                float out = acid1.processSample (input);
                if (activeSlope == Slope::TwentyFourDB)
                    out = acid2.processSample (out);
                return out;
            }

            case Type::MS20:
            {
                float out = ms201.processSample (input);
                if (activeSlope == Slope::TwentyFourDB)
                    out = ms202.processSample (out);
                return out;
            }

            default:
                return ladder.processSample (input);
        }
    }

    void reset()
    {
        ladder.reset();
        diode.reset();
        svf1.reset();
        svf2.reset();
        acid1.reset();
        acid2.reset();
        ms201.reset();
        ms202.reset();
    }

private:
    Type activeType = Type::Ladder;
    Slope activeSlope = Slope::TwentyFourDB;

    // 4-pole native filters (slope via internal stage tapping)
    LadderFilter ladder;
    DiodeLadderFilter diode;

    // 2-pole native filters (slope via cascading two instances)
    SVFilter svf1, svf2;
    AcidFilter acid1, acid2;
    MS20Filter ms201, ms202;

    void updateSlopeMode()
    {
        bool is12dB = (activeSlope == Slope::TwelveDB);

        // For 4-pole native filters: twoPole mode taps from stage[1]
        ladder.setTwoPole (is12dB);
        diode.setTwoPole (is12dB);
        // For 2-pole native filters: cascading is handled in processSample()
    }
};
