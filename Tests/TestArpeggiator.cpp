#include "catch_amalgamated.hpp"
#include "DSP/Arpeggiator.h"
#include <vector>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("Arpeggiator initial state", "[dsp]")
{
    Arpeggiator arp;
    arp.prepare (kSampleRate);

    SECTION ("Starts disabled")
    {
        REQUIRE_FALSE (arp.isEnabled());
    }

    SECTION ("Produces no events when disabled")
    {
        Arpeggiator::ArpEvent events[2];
        int count = arp.processSample (events);
        REQUIRE (count == 0);
    }
}

TEST_CASE ("Arpeggiator Up mode", "[dsp]")
{
    Arpeggiator arp;
    arp.prepare (kSampleRate);
    arp.setEnabled (true);
    arp.setMode (0); // Up
    arp.setBpm (120.0);
    arp.setSyncDivision (4); // 1/8 note
    arp.setOctaves (1);
    arp.setGate (0.75f);

    SECTION ("Produces note events after adding notes")
    {
        arp.noteOn (60, 0.8f);
        arp.noteOn (64, 0.7f);
        arp.noteOn (67, 0.6f);

        // Collect events over enough samples
        std::vector<Arpeggiator::ArpEvent> collectedEvents;
        int totalSamples = static_cast<int> (kSampleRate * 2.0); // 2 seconds

        for (int i = 0; i < totalSamples; ++i)
        {
            Arpeggiator::ArpEvent events[2];
            int count = arp.processSample (events);
            for (int j = 0; j < count; ++j)
                collectedEvents.push_back (events[j]);
        }

        // Should have note-on and note-off events
        REQUIRE (collectedEvents.size() > 0);

        // Check that note-ons cycle through C4, E4, G4
        std::vector<int> noteOns;
        for (auto& e : collectedEvents)
            if (e.isNoteOn)
                noteOns.push_back (e.midiNote);

        REQUIRE (noteOns.size() >= 3);
        // First three note-ons should be 60, 64, 67 (ascending)
        REQUIRE (noteOns[0] == 60);
        REQUIRE (noteOns[1] == 64);
        REQUIRE (noteOns[2] == 67);
    }
}

TEST_CASE ("Arpeggiator Down mode", "[dsp]")
{
    Arpeggiator arp;
    arp.prepare (kSampleRate);
    arp.setEnabled (true);
    arp.setMode (1); // Down
    arp.setBpm (120.0);
    arp.setSyncDivision (4); // 1/8
    arp.setOctaves (1);
    arp.setGate (0.75f);

    arp.noteOn (60, 0.8f);
    arp.noteOn (64, 0.7f);
    arp.noteOn (67, 0.6f);

    SECTION ("Notes play in descending order")
    {
        std::vector<int> noteOns;
        int totalSamples = static_cast<int> (kSampleRate * 2.0);

        for (int i = 0; i < totalSamples; ++i)
        {
            Arpeggiator::ArpEvent events[2];
            int count = arp.processSample (events);
            for (int j = 0; j < count; ++j)
                if (events[j].isNoteOn)
                    noteOns.push_back (events[j].midiNote);
        }

        REQUIRE (noteOns.size() >= 3);
        REQUIRE (noteOns[0] == 67);
        REQUIRE (noteOns[1] == 64);
        REQUIRE (noteOns[2] == 60);
    }
}

TEST_CASE ("Arpeggiator octave expansion", "[dsp]")
{
    Arpeggiator arp;
    arp.prepare (kSampleRate);
    arp.setEnabled (true);
    arp.setMode (0); // Up
    arp.setBpm (300.0); // fast for quicker collection
    arp.setSyncDivision (0); // 1/32 (fastest)
    arp.setOctaves (2);
    arp.setGate (0.5f);

    arp.noteOn (60, 0.8f);

    SECTION ("Plays across two octaves")
    {
        std::vector<int> noteOns;
        int totalSamples = static_cast<int> (kSampleRate * 2.0);

        for (int i = 0; i < totalSamples; ++i)
        {
            Arpeggiator::ArpEvent events[2];
            int count = arp.processSample (events);
            for (int j = 0; j < count; ++j)
                if (events[j].isNoteOn)
                    noteOns.push_back (events[j].midiNote);
        }

        REQUIRE (noteOns.size() >= 2);
        // Should play 60 and 72 (one octave up)
        bool has60 = false, has72 = false;
        for (int n : noteOns)
        {
            if (n == 60) has60 = true;
            if (n == 72) has72 = true;
        }
        REQUIRE (has60);
        REQUIRE (has72);
    }
}

TEST_CASE ("Arpeggiator note off clears sequence", "[dsp]")
{
    Arpeggiator arp;
    arp.prepare (kSampleRate);
    arp.setEnabled (true);
    arp.setMode (0);
    arp.setBpm (120.0);
    arp.setSyncDivision (4);

    arp.noteOn (60, 0.8f);
    arp.noteOff (60);

    SECTION ("No more note-ons after all notes released")
    {
        // Process a bit to let gate close
        for (int i = 0; i < 1000; ++i)
        {
            Arpeggiator::ArpEvent events[2];
            arp.processSample (events);
        }

        // Now should produce no more events
        int eventsAfter = 0;
        for (int i = 0; i < 10000; ++i)
        {
            Arpeggiator::ArpEvent events[2];
            int count = arp.processSample (events);
            for (int j = 0; j < count; ++j)
                if (events[j].isNoteOn)
                    ++eventsAfter;
        }
        REQUIRE (eventsAfter == 0);
    }
}

TEST_CASE ("Arpeggiator disable sends note off", "[dsp]")
{
    Arpeggiator arp;
    arp.prepare (kSampleRate);
    arp.setEnabled (true);
    arp.setMode (0);
    arp.setBpm (120.0);
    arp.setSyncDivision (4);
    arp.setGate (0.75f);

    arp.noteOn (60, 0.8f);

    // Process until we get a note on
    bool gotNoteOn = false;
    for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
    {
        Arpeggiator::ArpEvent events[2];
        int count = arp.processSample (events);
        for (int j = 0; j < count; ++j)
            if (events[j].isNoteOn)
                gotNoteOn = true;
        if (gotNoteOn)
            break;
    }
    REQUIRE (gotNoteOn);

    arp.setEnabled (false);

    SECTION ("Disabled arp emits note-off for current note")
    {
        Arpeggiator::ArpEvent events[2];
        int count = arp.processSample (events);
        // Should get a note-off event
        bool gotOff = false;
        for (int j = 0; j < count; ++j)
            if (!events[j].isNoteOn)
                gotOff = true;
        REQUIRE (gotOff);
    }
}

TEST_CASE ("Arpeggiator latch mode", "[dsp]")
{
    Arpeggiator arp;
    arp.prepare (kSampleRate);
    arp.setEnabled (true);
    arp.setMode (0);
    arp.setBpm (300.0);
    arp.setSyncDivision (0); // fast
    arp.setGate (0.5f);
    arp.setLatch (true);

    arp.noteOn (60, 0.8f);
    arp.noteOff (60);

    SECTION ("Notes continue after key release in latch mode")
    {
        int noteOns = 0;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            Arpeggiator::ArpEvent events[2];
            int count = arp.processSample (events);
            for (int j = 0; j < count; ++j)
                if (events[j].isNoteOn)
                    ++noteOns;
        }
        REQUIRE (noteOns > 5);
    }
}

TEST_CASE ("Arpeggiator allNotesOff", "[dsp]")
{
    Arpeggiator arp;
    arp.prepare (kSampleRate);
    arp.setEnabled (true);
    arp.setMode (0);
    arp.setBpm (120.0);
    arp.setSyncDivision (4);
    arp.setLatch (true);

    arp.noteOn (60, 0.8f);
    arp.noteOn (64, 0.7f);

    arp.allNotesOff();

    SECTION ("allNotesOff clears everything including latched notes")
    {
        // Skip any pending note-off events
        for (int i = 0; i < 100; ++i)
        {
            Arpeggiator::ArpEvent events[2];
            arp.processSample (events);
        }

        int noteOns = 0;
        for (int i = 0; i < 10000; ++i)
        {
            Arpeggiator::ArpEvent events[2];
            int count = arp.processSample (events);
            for (int j = 0; j < count; ++j)
                if (events[j].isNoteOn)
                    ++noteOns;
        }
        REQUIRE (noteOns == 0);
    }
}
