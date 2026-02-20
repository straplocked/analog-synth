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

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

class MidiLearnManager : public juce::Timer
{
public:
    explicit MidiLearnManager (juce::AudioProcessorValueTreeState& apvtsRef)
        : apvts (apvtsRef)
    {
    }

    ~MidiLearnManager() override
    {
        stopTimer();
    }

    // --- Audio thread (called from processBlock) ---
    bool handleCC (int /*channel*/, int ccNumber, int ccValue)
    {
        if (ccNumber < 0 || ccNumber > 127)
            return false;

        if (learningActive.load (std::memory_order_relaxed))
        {
            lastLearnedCC.store (ccNumber, std::memory_order_release);
            return true;
        }

        const juce::SpinLock::ScopedLockType lock (mappingLock);
        auto& mapping = ccToParam[static_cast<std::size_t> (ccNumber)];
        if (mapping.param != nullptr)
        {
            float normalized = static_cast<float> (ccValue) / 127.0f;
            mapping.param->setValueNotifyingHost (normalized);
            return true;
        }

        return false;
    }

    // --- Message thread (called from UI) ---
    void startLearning (const juce::String& paramID)
    {
        learningParamID = paramID;
        lastLearnedCC.store (-1, std::memory_order_relaxed);
        learningActive.store (true, std::memory_order_release);
        startTimer (50);
        notifyListeners();
    }

    void cancelLearning()
    {
        stopTimer();
        learningActive.store (false, std::memory_order_release);
        learningParamID.clear();
        notifyListeners();
    }

    bool isLearning() const { return learningActive.load (std::memory_order_acquire); }

    juce::String getLearningParamID() const { return learningParamID; }

    void assignCC (int ccNumber, const juce::String& paramID)
    {
        if (ccNumber < 0 || ccNumber > 127)
            return;

        auto* param = apvts.getParameter (paramID);
        if (param == nullptr)
            return;

        const juce::SpinLock::ScopedLockType lock (mappingLock);

        // Remove any existing mapping for this CC
        ccToParam[static_cast<std::size_t> (ccNumber)] = {};

        // Remove any existing mapping for this param (a param can only have one CC)
        for (auto& m : ccToParam)
        {
            if (m.paramID == paramID)
                m = {};
        }

        ccToParam[static_cast<std::size_t> (ccNumber)] = { paramID, param };
    }

    void removeMapping (const juce::String& paramID)
    {
        const juce::SpinLock::ScopedLockType lock (mappingLock);
        for (auto& m : ccToParam)
        {
            if (m.paramID == paramID)
            {
                m = {};
                break;
            }
        }
        notifyListeners();
    }

    void removeMappingForCC (int ccNumber)
    {
        if (ccNumber < 0 || ccNumber > 127)
            return;

        const juce::SpinLock::ScopedLockType lock (mappingLock);
        ccToParam[static_cast<std::size_t> (ccNumber)] = {};
        notifyListeners();
    }

    void clearAllMappings()
    {
        {
            const juce::SpinLock::ScopedLockType lock (mappingLock);
            for (auto& m : ccToParam)
                m = {};
        }
        saveToState();
        notifyListeners();
    }

    // --- Query ---
    int getCCForParam (const juce::String& paramID) const
    {
        const juce::SpinLock::ScopedLockType lock (mappingLock);
        for (int i = 0; i < 128; ++i)
        {
            if (ccToParam[static_cast<std::size_t> (i)].paramID == paramID)
                return i;
        }
        return -1;
    }

    juce::String getParamForCC (int ccNumber) const
    {
        if (ccNumber < 0 || ccNumber > 127)
            return {};

        const juce::SpinLock::ScopedLockType lock (mappingLock);
        return ccToParam[static_cast<std::size_t> (ccNumber)].paramID;
    }

    bool hasMapping (const juce::String& paramID) const
    {
        const juce::SpinLock::ScopedLockType lock (mappingLock);
        for (const auto& m : ccToParam)
        {
            if (m.paramID == paramID)
                return true;
        }
        return false;
    }

    // --- State persistence ---
    void saveToState()
    {
        auto existing = apvts.state.getChildWithName ("MIDI_LEARN_DATA");
        if (existing.isValid())
            apvts.state.removeChild (existing, nullptr);

        juce::ValueTree midiLearnData ("MIDI_LEARN_DATA");

        const juce::SpinLock::ScopedLockType lock (mappingLock);
        for (int i = 0; i < 128; ++i)
        {
            const auto& m = ccToParam[static_cast<std::size_t> (i)];
            if (m.paramID.isNotEmpty())
            {
                juce::ValueTree mapEntry ("MAP");
                mapEntry.setProperty ("cc", i, nullptr);
                mapEntry.setProperty ("param", m.paramID, nullptr);
                midiLearnData.addChild (mapEntry, -1, nullptr);
            }
        }

        apvts.state.addChild (midiLearnData, -1, nullptr);
    }

    void loadFromState()
    {
        auto midiLearnData = apvts.state.getChildWithName ("MIDI_LEARN_DATA");

        const juce::SpinLock::ScopedLockType lock (mappingLock);
        for (auto& m : ccToParam)
            m = {};

        if (! midiLearnData.isValid())
            return;

        for (int i = 0; i < midiLearnData.getNumChildren(); ++i)
        {
            auto mapEntry = midiLearnData.getChild (i);
            if (mapEntry.hasType ("MAP"))
            {
                int cc = static_cast<int> (mapEntry.getProperty ("cc", -1));
                juce::String paramID = mapEntry.getProperty ("param", "").toString();

                if (cc >= 0 && cc < 128 && paramID.isNotEmpty())
                {
                    auto* param = apvts.getParameter (paramID);
                    if (param != nullptr)
                        ccToParam[static_cast<std::size_t> (cc)] = { paramID, param };
                }
            }
        }

        notifyListeners();
    }

    // --- Listener interface ---
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void midiLearnStateChanged() = 0;
    };

    void addListener (Listener* l) { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    void timerCallback() override
    {
        int cc = lastLearnedCC.load (std::memory_order_acquire);
        if (cc >= 0)
        {
            stopTimer();
            learningActive.store (false, std::memory_order_release);
            assignCC (cc, learningParamID);
            learningParamID.clear();
            lastLearnedCC.store (-1, std::memory_order_relaxed);
            saveToState();
            notifyListeners();
        }
    }

    void notifyListeners()
    {
        listeners.call ([] (Listener& l) { l.midiLearnStateChanged(); });
    }

    juce::AudioProcessorValueTreeState& apvts;

    struct Mapping
    {
        juce::String paramID;
        juce::RangedAudioParameter* param = nullptr;
    };

    std::array<Mapping, 128> ccToParam;
    mutable juce::SpinLock mappingLock;

    std::atomic<bool> learningActive { false };
    juce::String learningParamID;
    std::atomic<int> lastLearnedCC { -1 };

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiLearnManager)
};
