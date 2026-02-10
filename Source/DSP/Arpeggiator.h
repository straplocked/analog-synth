#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>

class Arpeggiator
{
public:
    enum class Mode { Up, Down, UpDown, Random, AsPlayed };

    struct ArpEvent
    {
        int midiNote = -1;
        float velocity = 0.0f;
        bool isNoteOn = false;
    };

    Arpeggiator() = default;

    void prepare (double newSampleRate);
    void reset();

    // Parameter setters (called once per processBlock)
    void setEnabled (bool newEnabled);
    void setMode (int modeIndex);
    void setSyncDivision (int divIndex);
    void setOctaves (int numOctaves);
    void setGate (float gateFraction);
    void setLatch (bool latchEnabled);
    void setBpm (double newBpm);

    // MIDI input
    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();

    // Per-sample timing — returns 0, 1, or 2 events
    int processSample (ArpEvent* events);

    bool isEnabled() const { return enabled; }

private:
    struct HeldNote
    {
        int midiNote;
        float velocity;
        int insertionOrder;
    };

    void buildSequence();
    int advanceSequenceIndex();
    void recalculateTiming();
    double syncDivisionToBeats (int divIndex) const;
    int nextRandom (int count);

    double sampleRate = 44100.0;
    bool enabled = false;
    Mode mode = Mode::Up;
    int syncDivIndex = 4;
    int octaves = 1;
    float gate = 0.75f;
    bool latchOn = false;
    double bpm = 120.0;

    // Held note tracking
    std::vector<HeldNote> heldNotes;
    std::vector<HeldNote> latchedNotes;
    int nextInsertionOrder = 0;
    bool anyKeysPhysicallyHeld = false;

    // Sequence
    std::vector<HeldNote> sequence;
    int sequenceIndex = -1;
    bool goingUp = true;

    // Timing
    int samplesPerStep = 0;
    int sampleCounter = 0;
    int gateOffSample = 0;
    bool gateOpen = false;
    int currentArpNote = -1;
    float currentArpVelocity = 0.0f;

    // Random state
    uint32_t randomState = 12345;
    int lastRandomIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Arpeggiator)
};
