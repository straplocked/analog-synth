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
#include "DSP/ModMatrix.h"
#include <cmath>

// ---- ModSlot ----

TEST_CASE ("ModSlot isActive", "[modmatrix]")
{
    ModSlot slot;
    REQUIRE_FALSE (slot.isActive());

    SECTION ("Active when source, dest, and amount are all set")
    {
        slot.source = ModSource::LFO1;
        slot.dest = ModDest::LPFCutoff;
        slot.amount = 0.5f;
        REQUIRE (slot.isActive());
    }

    SECTION ("Inactive with source = None")
    {
        slot.source = ModSource::None;
        slot.dest = ModDest::LPFCutoff;
        slot.amount = 0.5f;
        REQUIRE_FALSE (slot.isActive());
    }

    SECTION ("Inactive with dest = None")
    {
        slot.source = ModSource::LFO1;
        slot.dest = ModDest::None;
        slot.amount = 0.5f;
        REQUIRE_FALSE (slot.isActive());
    }

    SECTION ("Inactive with amount = 0")
    {
        slot.source = ModSource::LFO1;
        slot.dest = ModDest::LPFCutoff;
        slot.amount = 0.0f;
        REQUIRE_FALSE (slot.isActive());
    }
}

// ---- ModMatrix setSlot ----

TEST_CASE ("ModMatrix setSlot stores values", "[modmatrix]")
{
    ModMatrix matrix;

    matrix.setSlot (0, static_cast<int> (ModSource::FilterEnv),
                    static_cast<int> (ModDest::Osc1Freq), 0.75f);
    matrix.prepareDestinationMask();

    auto& slot = matrix.getSlot (0);
    REQUIRE (slot.source == ModSource::FilterEnv);
    REQUIRE (slot.dest == ModDest::Osc1Freq);
    REQUIRE (slot.amount == Catch::Approx (0.75f));
}

TEST_CASE ("ModMatrix setSlot clamps amount to [-1, 1]", "[modmatrix]")
{
    ModMatrix matrix;

    matrix.setSlot (0, static_cast<int> (ModSource::LFO1),
                    static_cast<int> (ModDest::LPFCutoff), 2.5f);
    REQUIRE (matrix.getSlot (0).amount == Catch::Approx (1.0f));

    matrix.setSlot (0, static_cast<int> (ModSource::LFO1),
                    static_cast<int> (ModDest::LPFCutoff), -3.0f);
    REQUIRE (matrix.getSlot (0).amount == Catch::Approx (-1.0f));
}

TEST_CASE ("ModMatrix setSlot ignores out-of-range index", "[modmatrix]")
{
    ModMatrix matrix;
    matrix.setSlot (-1, 1, 1, 0.5f);
    matrix.setSlot (16, 1, 1, 0.5f);
    // Should not crash; slots should remain at defaults
    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
        REQUIRE_FALSE (matrix.getSlot (i).isActive());
}

// ---- Destination bitmask ----

TEST_CASE ("ModMatrix hasModulationFor reflects active slots", "[modmatrix]")
{
    ModMatrix matrix;

    SECTION ("No slots active — no destinations have modulation")
    {
        matrix.prepareDestinationMask();
        REQUIRE_FALSE (matrix.hasModulationFor (ModDest::LPFCutoff));
        REQUIRE_FALSE (matrix.hasModulationFor (ModDest::Osc1Freq));
    }

    SECTION ("Single slot active — only that destination is flagged")
    {
        matrix.setSlot (0, static_cast<int> (ModSource::LFO1),
                        static_cast<int> (ModDest::LPFCutoff), 0.5f);
        matrix.prepareDestinationMask();

        REQUIRE (matrix.hasModulationFor (ModDest::LPFCutoff));
        REQUIRE_FALSE (matrix.hasModulationFor (ModDest::Osc1Freq));
        REQUIRE_FALSE (matrix.hasModulationFor (ModDest::AmpLevel));
    }

    SECTION ("Multiple slots — all targeted destinations are flagged")
    {
        matrix.setSlot (0, static_cast<int> (ModSource::LFO1),
                        static_cast<int> (ModDest::LPFCutoff), 0.5f);
        matrix.setSlot (1, static_cast<int> (ModSource::Env3),
                        static_cast<int> (ModDest::Osc1Freq), -0.3f);
        matrix.setSlot (2, static_cast<int> (ModSource::Macro1),
                        static_cast<int> (ModDest::ReverbMix), 1.0f);
        matrix.prepareDestinationMask();

        REQUIRE (matrix.hasModulationFor (ModDest::LPFCutoff));
        REQUIRE (matrix.hasModulationFor (ModDest::Osc1Freq));
        REQUIRE (matrix.hasModulationFor (ModDest::ReverbMix));
        REQUIRE_FALSE (matrix.hasModulationFor (ModDest::ChorusMix));
    }
}

// ---- computeModulation ----

TEST_CASE ("ModMatrix computeModulation returns 0 for no routings", "[modmatrix]")
{
    ModMatrix matrix;
    matrix.prepareDestinationMask();

    VoiceModValues voice;
    GlobalModValues global;

    REQUIRE (matrix.computeModulation (ModDest::LPFCutoff, voice, global) == Catch::Approx (0.0f));
}

TEST_CASE ("ModMatrix computeModulation single slot", "[modmatrix]")
{
    ModMatrix matrix;
    VoiceModValues voice;
    GlobalModValues global;

    voice.lfo1 = 0.8f;
    matrix.setSlot (0, static_cast<int> (ModSource::LFO1),
                    static_cast<int> (ModDest::LPFCutoff), 0.5f);
    matrix.prepareDestinationMask();

    float result = matrix.computeModulation (ModDest::LPFCutoff, voice, global);
    REQUIRE (result == Catch::Approx (0.8f * 0.5f));
}

TEST_CASE ("ModMatrix computeModulation sums multiple slots to same dest", "[modmatrix]")
{
    ModMatrix matrix;
    VoiceModValues voice;
    GlobalModValues global;

    voice.lfo1 = 1.0f;
    voice.filterEnv = 0.5f;
    global.modWheel = 0.7f;

    // Three slots all targeting LPFCutoff
    matrix.setSlot (0, static_cast<int> (ModSource::LFO1),
                    static_cast<int> (ModDest::LPFCutoff), 0.3f);
    matrix.setSlot (1, static_cast<int> (ModSource::FilterEnv),
                    static_cast<int> (ModDest::LPFCutoff), -0.5f);
    matrix.setSlot (2, static_cast<int> (ModSource::ModWheel),
                    static_cast<int> (ModDest::LPFCutoff), 1.0f);
    matrix.prepareDestinationMask();

    float expected = (1.0f * 0.3f) + (0.5f * -0.5f) + (0.7f * 1.0f);
    float result = matrix.computeModulation (ModDest::LPFCutoff, voice, global);
    REQUIRE (result == Catch::Approx (expected));
}

TEST_CASE ("ModMatrix computeModulation ignores slots targeting other dests", "[modmatrix]")
{
    ModMatrix matrix;
    VoiceModValues voice;
    GlobalModValues global;

    voice.lfo1 = 1.0f;
    voice.filterEnv = 1.0f;

    matrix.setSlot (0, static_cast<int> (ModSource::LFO1),
                    static_cast<int> (ModDest::LPFCutoff), 0.5f);
    matrix.setSlot (1, static_cast<int> (ModSource::FilterEnv),
                    static_cast<int> (ModDest::Osc1Freq), 0.8f);
    matrix.prepareDestinationMask();

    float result = matrix.computeModulation (ModDest::LPFCutoff, voice, global);
    REQUIRE (result == Catch::Approx (1.0f * 0.5f));

    result = matrix.computeModulation (ModDest::Osc1Freq, voice, global);
    REQUIRE (result == Catch::Approx (1.0f * 0.8f));
}

// ---- Source value routing ----

TEST_CASE ("ModMatrix routes all voice sources correctly", "[modmatrix]")
{
    ModMatrix matrix;
    VoiceModValues voice;
    GlobalModValues global;

    voice.filterEnv = 0.1f;
    voice.ampEnv = 0.2f;
    voice.env3 = 0.3f;
    voice.env4 = 0.4f;
    voice.lfo1 = 0.5f;
    voice.lfo2 = 0.6f;
    voice.lfo3 = 0.7f;
    voice.velocity = 0.8f;
    voice.noteNumber = 0.9f;
    voice.random = 0.15f;

    struct TestCase { ModSource src; float expected; };
    TestCase cases[] = {
        { ModSource::FilterEnv,  0.1f },
        { ModSource::AmpEnv,     0.2f },
        { ModSource::Env3,       0.3f },
        { ModSource::Env4,       0.4f },
        { ModSource::LFO1,       0.5f },
        { ModSource::LFO2,       0.6f },
        { ModSource::LFO3,       0.7f },
        { ModSource::Velocity,   0.8f },
        { ModSource::NoteNumber, 0.9f },
        { ModSource::Random,     0.15f },
    };

    for (int i = 0; i < 10; ++i)
    {
        matrix.setSlot (i, static_cast<int> (cases[i].src),
                        static_cast<int> (ModDest::LPFCutoff), 1.0f);
    }
    matrix.prepareDestinationMask();

    // computeModulation sums all of them (amount=1.0 each)
    float total = matrix.computeModulation (ModDest::LPFCutoff, voice, global);
    float expectedTotal = 0.1f + 0.2f + 0.3f + 0.4f + 0.5f + 0.6f + 0.7f + 0.8f + 0.9f + 0.15f;
    REQUIRE (total == Catch::Approx (expectedTotal));
}

TEST_CASE ("ModMatrix routes all global sources correctly", "[modmatrix]")
{
    ModMatrix matrix;
    VoiceModValues voice;
    GlobalModValues global;

    global.modWheel = 0.1f;
    global.aftertouch = 0.2f;
    global.pitchBend = -0.3f;
    global.macro1 = 0.4f;
    global.macro2 = 0.5f;
    global.macro3 = 0.6f;
    global.macro4 = 0.7f;

    matrix.setSlot (0, static_cast<int> (ModSource::ModWheel),
                    static_cast<int> (ModDest::AmpLevel), 1.0f);
    matrix.setSlot (1, static_cast<int> (ModSource::Aftertouch),
                    static_cast<int> (ModDest::AmpLevel), 1.0f);
    matrix.setSlot (2, static_cast<int> (ModSource::PitchBend),
                    static_cast<int> (ModDest::AmpLevel), 1.0f);
    matrix.setSlot (3, static_cast<int> (ModSource::Macro1),
                    static_cast<int> (ModDest::AmpLevel), 1.0f);
    matrix.setSlot (4, static_cast<int> (ModSource::Macro2),
                    static_cast<int> (ModDest::AmpLevel), 1.0f);
    matrix.setSlot (5, static_cast<int> (ModSource::Macro3),
                    static_cast<int> (ModDest::AmpLevel), 1.0f);
    matrix.setSlot (6, static_cast<int> (ModSource::Macro4),
                    static_cast<int> (ModDest::AmpLevel), 1.0f);
    matrix.prepareDestinationMask();

    float total = matrix.computeModulation (ModDest::AmpLevel, voice, global);
    float expected = 0.1f + 0.2f + (-0.3f) + 0.4f + 0.5f + 0.6f + 0.7f;
    REQUIRE (total == Catch::Approx (expected));
}

// ---- Negative amount (bipolar) ----

TEST_CASE ("ModMatrix supports negative amounts", "[modmatrix]")
{
    ModMatrix matrix;
    VoiceModValues voice;
    GlobalModValues global;

    voice.lfo1 = 1.0f;
    matrix.setSlot (0, static_cast<int> (ModSource::LFO1),
                    static_cast<int> (ModDest::Osc1Freq), -0.75f);
    matrix.prepareDestinationMask();

    float result = matrix.computeModulation (ModDest::Osc1Freq, voice, global);
    REQUIRE (result == Catch::Approx (-0.75f));
}

// ---- Destination bitmask coverage ----

TEST_CASE ("ModMatrix bitmask works for high-index destinations", "[modmatrix]")
{
    ModMatrix matrix;
    // ReverbMix is index 31, testing near the edge of uint64_t
    matrix.setSlot (0, static_cast<int> (ModSource::Macro4),
                    static_cast<int> (ModDest::ReverbMix), 0.5f);
    matrix.prepareDestinationMask();

    REQUIRE (matrix.hasModulationFor (ModDest::ReverbMix));
    REQUIRE_FALSE (matrix.hasModulationFor (ModDest::ReverbSize));
}

// ---- Overwriting slots ----

TEST_CASE ("ModMatrix setSlot overwrites previous value", "[modmatrix]")
{
    ModMatrix matrix;

    matrix.setSlot (3, static_cast<int> (ModSource::LFO1),
                    static_cast<int> (ModDest::LPFCutoff), 0.5f);
    matrix.setSlot (3, static_cast<int> (ModSource::Env3),
                    static_cast<int> (ModDest::Osc1Freq), -0.2f);
    matrix.prepareDestinationMask();

    REQUIRE (matrix.getSlot (3).source == ModSource::Env3);
    REQUIRE (matrix.getSlot (3).dest == ModDest::Osc1Freq);
    REQUIRE (matrix.getSlot (3).amount == Catch::Approx (-0.2f));

    REQUIRE_FALSE (matrix.hasModulationFor (ModDest::LPFCutoff));
    REQUIRE (matrix.hasModulationFor (ModDest::Osc1Freq));
}
