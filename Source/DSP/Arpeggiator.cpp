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

#include "Arpeggiator.h"

void Arpeggiator::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void Arpeggiator::reset()
{
    heldNotes.clear();
    latchedNotes.clear();
    sequence.clear();
    sequenceIndex = -1;
    goingUp = true;
    sampleCounter = 0;
    gateOpen = false;
    currentArpNote = -1;
    currentArpVelocity = 0.0f;
    anyKeysPhysicallyHeld = false;
    nextInsertionOrder = 0;
    lastRandomIndex = -1;
    recalculateTiming();
}

// ---- Parameter setters ----

void Arpeggiator::setEnabled (bool newEnabled)
{
    if (enabled == newEnabled)
        return;

    enabled = newEnabled;

    if (! enabled)
    {
        // Turning off — next processSample will emit noteOff for current note
        heldNotes.clear();
        latchedNotes.clear();
        sequence.clear();
        sequenceIndex = -1;
        sampleCounter = 0;
        goingUp = true;
        anyKeysPhysicallyHeld = false;
    }
    else
    {
        sampleCounter = 0;
        sequenceIndex = -1;
        goingUp = true;
    }
}

void Arpeggiator::setMode (int modeIndex)
{
    auto newMode = static_cast<Mode> (std::clamp (modeIndex, 0, 4));
    if (mode != newMode)
    {
        mode = newMode;
        goingUp = true;
        buildSequence();
    }
}

void Arpeggiator::setSyncDivision (int divIndex)
{
    int clamped = std::clamp (divIndex, 0, 8);
    if (syncDivIndex != clamped)
    {
        syncDivIndex = clamped;
        recalculateTiming();
    }
}

void Arpeggiator::setOctaves (int numOctaves)
{
    int clamped = std::clamp (numOctaves, 1, 4);
    if (octaves != clamped)
    {
        octaves = clamped;
        buildSequence();
    }
}

void Arpeggiator::setGate (float gateFraction)
{
    gate = std::clamp (gateFraction, 0.1f, 1.0f);
    gateOffSample = std::max (1, static_cast<int> (static_cast<float> (samplesPerStep) * gate));
}

void Arpeggiator::setLatch (bool latchEnabled)
{
    if (latchOn == latchEnabled)
        return;

    latchOn = latchEnabled;

    if (! latchOn)
    {
        latchedNotes.clear();
        buildSequence();
    }
}

void Arpeggiator::setBpm (double newBpm)
{
    if (std::abs (bpm - newBpm) > 0.01)
    {
        bpm = std::max (1.0, newBpm);
        recalculateTiming();
    }
}

// ---- MIDI input ----

void Arpeggiator::noteOn (int midiNote, float velocity)
{
    // Remove duplicate if re-pressed
    heldNotes.erase (
        std::remove_if (heldNotes.begin(), heldNotes.end(),
                        [midiNote] (const HeldNote& n) { return n.midiNote == midiNote; }),
        heldNotes.end());

    heldNotes.push_back ({ midiNote, velocity, nextInsertionOrder++ });
    anyKeysPhysicallyHeld = true;

    if (latchOn)
    {
        // Remove duplicate from latched notes too
        latchedNotes.erase (
            std::remove_if (latchedNotes.begin(), latchedNotes.end(),
                            [midiNote] (const HeldNote& n) { return n.midiNote == midiNote; }),
            latchedNotes.end());
        latchedNotes.push_back ({ midiNote, velocity, nextInsertionOrder - 1 });
    }

    buildSequence();

    // Retrigger: force next step immediately
    sampleCounter = samplesPerStep;
    goingUp = true;
    sequenceIndex = -1;
}

void Arpeggiator::noteOff (int midiNote)
{
    heldNotes.erase (
        std::remove_if (heldNotes.begin(), heldNotes.end(),
                        [midiNote] (const HeldNote& n) { return n.midiNote == midiNote; }),
        heldNotes.end());

    anyKeysPhysicallyHeld = ! heldNotes.empty();

    if (! latchOn)
        buildSequence();

    // In latch mode, latchedNotes persist — sequence unchanged
}

void Arpeggiator::allNotesOff()
{
    heldNotes.clear();
    latchedNotes.clear();
    anyKeysPhysicallyHeld = false;
    buildSequence();
}

// ---- Per-sample timing engine ----

int Arpeggiator::processSample (ArpEvent* events)
{
    int eventCount = 0;

    if (! enabled || sequence.empty())
    {
        if (gateOpen && currentArpNote >= 0)
        {
            events[0] = { currentArpNote, 0.0f, false };
            gateOpen = false;
            currentArpNote = -1;
            return 1;
        }
        return 0;
    }

    sampleCounter++;

    // Gate-off within a step (not at step boundary)
    if (gateOpen && sampleCounter >= gateOffSample && sampleCounter < samplesPerStep)
    {
        events[eventCount++] = { currentArpNote, 0.0f, false };
        gateOpen = false;
        currentArpNote = -1;
    }

    // Step boundary — advance to next note
    if (sampleCounter >= samplesPerStep)
    {
        sampleCounter = 0;

        // Close gate if still open (gate == 1.0 legato case)
        if (gateOpen && currentArpNote >= 0)
        {
            events[eventCount++] = { currentArpNote, 0.0f, false };
            gateOpen = false;
        }

        int idx = advanceSequenceIndex();
        if (idx >= 0 && idx < static_cast<int> (sequence.size()))
        {
            currentArpNote = sequence[static_cast<size_t> (idx)].midiNote;
            currentArpVelocity = sequence[static_cast<size_t> (idx)].velocity;
            events[eventCount++] = { currentArpNote, currentArpVelocity, true };
            gateOpen = true;
        }
    }

    return eventCount;
}

// ---- Internal ----

void Arpeggiator::buildSequence()
{
    const auto& sourceNotes = (latchOn && ! latchedNotes.empty()) ? latchedNotes : heldNotes;
    sequence.clear();

    if (sourceNotes.empty())
        return;

    auto sorted = sourceNotes;

    if (mode == Mode::AsPlayed)
        std::sort (sorted.begin(), sorted.end(),
                   [] (const HeldNote& a, const HeldNote& b) { return a.insertionOrder < b.insertionOrder; });
    else
        std::sort (sorted.begin(), sorted.end(),
                   [] (const HeldNote& a, const HeldNote& b) { return a.midiNote < b.midiNote; });

    // Expand across octaves
    for (int oct = 0; oct < octaves; ++oct)
    {
        for (const auto& note : sorted)
        {
            HeldNote expanded = note;
            expanded.midiNote = note.midiNote + oct * 12;
            if (expanded.midiNote <= 127)
                sequence.push_back (expanded);
        }
    }

    // Reverse for Down mode
    if (mode == Mode::Down)
        std::reverse (sequence.begin(), sequence.end());

    // Clamp sequence index
    if (sequence.empty())
        sequenceIndex = -1;
    else if (sequenceIndex >= static_cast<int> (sequence.size()))
        sequenceIndex = static_cast<int> (sequence.size()) - 1;
}

int Arpeggiator::advanceSequenceIndex()
{
    if (sequence.empty())
        return -1;

    int count = static_cast<int> (sequence.size());

    if (mode == Mode::Random)
        return nextRandom (count);

    if (mode == Mode::UpDown)
    {
        if (count == 1)
        {
            sequenceIndex = 0;
            return 0;
        }

        if (goingUp)
        {
            sequenceIndex++;
            if (sequenceIndex >= count - 1)
            {
                sequenceIndex = count - 1;
                goingUp = false;
            }
        }
        else
        {
            sequenceIndex--;
            if (sequenceIndex <= 0)
            {
                sequenceIndex = 0;
                goingUp = true;
            }
        }
        return sequenceIndex;
    }

    // Up, Down, AsPlayed: simple wrap-around
    sequenceIndex = (sequenceIndex + 1) % count;
    return sequenceIndex;
}

void Arpeggiator::recalculateTiming()
{
    double beatsPerStep = syncDivisionToBeats (syncDivIndex);
    double secondsPerBeat = 60.0 / bpm;
    double secondsPerStep = beatsPerStep * secondsPerBeat;
    samplesPerStep = std::max (1, static_cast<int> (secondsPerStep * sampleRate));
    gateOffSample = std::max (1, static_cast<int> (static_cast<float> (samplesPerStep) * gate));
}

double Arpeggiator::syncDivisionToBeats (int divIndex) const
{
    // Matches ParameterLayout order:
    // "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 Bar"
    static constexpr double divisions[] = {
        0.125,          // 1/32
        1.0 / 6.0,     // 1/16T
        0.25,           // 1/16
        1.0 / 3.0,     // 1/8T
        0.5,            // 1/8
        2.0 / 3.0,     // 1/4T
        1.0,            // 1/4
        2.0,            // 1/2
        4.0             // 1 Bar (4/4)
    };
    return divisions[std::clamp (divIndex, 0, 8)];
}

int Arpeggiator::nextRandom (int count)
{
    if (count <= 1)
        return 0;

    int idx;
    int attempts = 0;
    do
    {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        idx = static_cast<int> (randomState % static_cast<uint32_t> (count));
        attempts++;
    } while (idx == lastRandomIndex && attempts < 10);

    lastRandomIndex = idx;
    sequenceIndex = idx;
    return idx;
}
