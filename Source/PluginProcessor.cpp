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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterLayout.h"
#include "Utility/DebugLog.h"

AnalogSynthProcessor::AnalogSynthProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", ParameterLayout::createLayout())
{
    // Initialize LFO shape data child tree if not present
    initLFOShapeData();

    auto logPath = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                       .getChildFile ("AnalogSynth_debug.log")
                       .getFullPathName();
    DebugLog::instance().open (logPath);
    SYNTH_LOG ("AnalogSynth starting up");
}

void AnalogSynthProcessor::initLFOShapeData()
{
    if (! apvts.state.getChildWithName ("LFO_SHAPE_DATA").isValid())
    {
        juce::ValueTree shapeData ("LFO_SHAPE_DATA");

        for (int lfoIdx = 0; lfoIdx < 3; ++lfoIdx)
        {
            juce::String prefix = "LFO" + juce::String (lfoIdx);

            // Custom shape: default is ramp -1 to +1
            juce::ValueTree custom (juce::Identifier (prefix + "_CUSTOM"));
            custom.setProperty ("numPoints", 2, nullptr);
            custom.setProperty ("x0", 0.0f, nullptr);
            custom.setProperty ("y0", -1.0f, nullptr);
            custom.setProperty ("x1", 1.0f, nullptr);
            custom.setProperty ("y1", 1.0f, nullptr);
            shapeData.addChild (custom, -1, nullptr);

            // Step sequencer: default is 16 steps all at 0
            juce::ValueTree stepSeq (juce::Identifier (prefix + "_STEPSEQ"));
            stepSeq.setProperty ("numSteps", 16, nullptr);
            for (int s = 0; s < 16; ++s)
                stepSeq.setProperty (juce::Identifier ("s" + juce::String (s)), 0.0f, nullptr);
            shapeData.addChild (stepSeq, -1, nullptr);
        }

        apvts.state.addChild (shapeData, -1, nullptr);
    }
}

AnalogSynthProcessor::~AnalogSynthProcessor()
{
    SYNTH_LOG ("AnalogSynth shutting down");
    DebugLog::instance().close();
}

const juce::String AnalogSynthProcessor::getName() const { return JucePlugin_Name; }
bool AnalogSynthProcessor::acceptsMidi() const { return true; }
bool AnalogSynthProcessor::producesMidi() const { return false; }
bool AnalogSynthProcessor::isMidiEffect() const { return false; }
double AnalogSynthProcessor::getTailLengthSeconds() const { return 0.0; }

int AnalogSynthProcessor::getNumPrograms() { return 1; }
int AnalogSynthProcessor::getCurrentProgram() { return 0; }
void AnalogSynthProcessor::setCurrentProgram (int) {}
const juce::String AnalogSynthProcessor::getProgramName (int) { return {}; }
void AnalogSynthProcessor::changeProgramName (int, const juce::String&) {}

void AnalogSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    SYNTH_LOG ("prepareToPlay: sr=" + juce::String (sampleRate)
               + " blockSize=" + juce::String (samplesPerBlock));
    voiceManager.prepare (sampleRate, samplesPerBlock);
    arpeggiator.prepare (sampleRate);
    fxChain.prepare (sampleRate, samplesPerBlock);
}

void AnalogSynthProcessor::releaseResources() {}

bool AnalogSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void AnalogSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();

    buffer.clear();

    // Merge on-screen keyboard MIDI into the buffer
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);

    // ---- Tempo extraction ----
    double currentBpm = static_cast<double> (apvts.getRawParameterValue (ParameterIDs::arpStandaloneBpm)->load());
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto hostBpm = pos->getBpm())
            {
                if (*hostBpm > 0.0)
                    currentBpm = *hostBpm;
            }
        }
    }

    // ---- Update arp parameters ----
    bool arpEnabled = apvts.getRawParameterValue (ParameterIDs::arpEnabled)->load() > 0.5f;
    arpeggiator.setEnabled (arpEnabled);
    arpeggiator.setMode (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::arpMode)->load()));
    arpeggiator.setSyncDivision (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::arpSyncDiv)->load()));
    arpeggiator.setOctaves (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::arpOctaves)->load()));
    arpeggiator.setGate (apvts.getRawParameterValue (ParameterIDs::arpGate)->load());
    arpeggiator.setLatch (apvts.getRawParameterValue (ParameterIDs::arpLatch)->load() > 0.5f);
    arpeggiator.setBpm (currentBpm);

    // Pass BPM to voice manager for LFO tempo sync
    voiceManager.setBpm (currentBpm);

    // Update all voice parameters from APVTS (includes per-voice drift)
    voiceManager.updateParameters (apvts);

    // ---- MIDI + Arp processing ----
    if (arpEnabled)
    {
        // Feed note events to arpeggiator; non-note events go to VoiceManager
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                SYNTH_LOG ("ARP noteOn: " + juce::String (msg.getNoteNumber())
                           + " vel=" + juce::String (msg.getFloatVelocity(), 2));
                arpeggiator.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
            }
            else if (msg.isNoteOff())
            {
                SYNTH_LOG ("ARP noteOff: " + juce::String (msg.getNoteNumber()));
                arpeggiator.noteOff (msg.getNoteNumber());
            }
            else if (msg.isPitchWheel())
            {
                float normalised = (msg.getPitchWheelValue() - 8192) / 8192.0f;
                voiceManager.setPitchBend (normalised);
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                voiceManager.setModWheel (msg.getControllerValue() / 127.0f);
            }
            else if (msg.isChannelPressure())
            {
                voiceManager.setAftertouch (msg.getChannelPressureValue() / 127.0f);
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                arpeggiator.allNotesOff();
                voiceManager.allNotesOff();
            }
        }

        // Run arp timing engine and route events to VoiceManager
        Arpeggiator::ArpEvent arpEvents[3];
        for (int i = 0; i < numSamples; ++i)
        {
            int count = arpeggiator.processSample (arpEvents);
            for (int e = 0; e < count; ++e)
            {
                if (arpEvents[e].isNoteOn)
                    voiceManager.noteOn (arpEvents[e].midiNote, arpEvents[e].velocity);
                else
                    voiceManager.noteOff (arpEvents[e].midiNote);
            }
        }
    }
    else
    {
        // Arp disabled — pass MIDI directly to VoiceManager
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                SYNTH_LOG ("MIDI noteOn: " + juce::String (msg.getNoteNumber())
                           + " vel=" + juce::String (msg.getFloatVelocity(), 2));
                voiceManager.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
            }
            else if (msg.isNoteOff())
            {
                SYNTH_LOG ("MIDI noteOff: " + juce::String (msg.getNoteNumber()));
                voiceManager.noteOff (msg.getNoteNumber());
            }
            else if (msg.isPitchWheel())
            {
                float normalised = (msg.getPitchWheelValue() - 8192) / 8192.0f;
                voiceManager.setPitchBend (normalised);
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                voiceManager.setModWheel (msg.getControllerValue() / 127.0f);
            }
            else if (msg.isChannelPressure())
            {
                voiceManager.setAftertouch (msg.getChannelPressureValue() / 127.0f);
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                voiceManager.allNotesOff();
        }
    }

    // Read on-screen wheel values (override MIDI if non-default)
    if (auto* editor = dynamic_cast<AnalogSynthEditor*> (getActiveEditor()))
    {
        float uiPitch = editor->getPitchBend();
        float uiMod   = editor->getModWheel();
        if (uiPitch != 0.0f)
            voiceManager.setPitchBend (uiPitch);
        if (uiMod > 0.0f)
            voiceManager.setModWheel (uiMod);
    }

    // Render all voices (sums to stereo buffer)
    voiceManager.renderBlock (buffer, numSamples);

    // Check for NaN/Inf after voice render
    {
        float maxVal = 0.0f;
        bool hasNaN = false;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                if (std::isnan (data[i]) || std::isinf (data[i]))
                    hasNaN = true;
                maxVal = std::max (maxVal, std::abs (data[i]));
            }
        }
        if (hasNaN)
        {
            SYNTH_LOG ("!!! NaN/Inf detected AFTER voiceManager.renderBlock! Clearing buffer.");
            buffer.clear();
        }
        else if (maxVal > 10.0f)
        {
            SYNTH_LOG ("WARNING: peak after voices = " + juce::String (maxVal, 2));
        }
    }

    // Effects chain (stereo)
    fxChain.setBpm (currentBpm);
    fxChain.updateParameters (apvts);
    fxChain.processBlock (buffer);

    // Check for NaN/Inf after FX
    {
        bool hasNaN = false;
        float maxVal = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                if (std::isnan (data[i]) || std::isinf (data[i]))
                    hasNaN = true;
                maxVal = std::max (maxVal, std::abs (data[i]));
            }
        }
        if (hasNaN)
        {
            SYNTH_LOG ("!!! NaN/Inf detected AFTER fxChain.processBlock! Clearing buffer.");
            buffer.clear();
        }
        else if (maxVal > 10.0f)
        {
            SYNTH_LOG ("WARNING: peak after FX = " + juce::String (maxVal, 2));
        }
    }

    // Apply master volume
    float masterDb = apvts.getRawParameterValue (ParameterIDs::masterVol)->load();
    float masterGain = juce::Decibels::decibelsToGain (masterDb);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::multiply (buffer.getWritePointer (ch), masterGain, numSamples);

    // Push samples to waveform display (mono mix of L channel)
    if (auto* editor = dynamic_cast<AnalogSynthEditor*> (getActiveEditor()))
        editor->getWaveformDisplay().pushSamples (buffer.getReadPointer (0), numSamples);
}

bool AnalogSynthProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AnalogSynthProcessor::createEditor()
{
    return new AnalogSynthEditor (*this);
}

void AnalogSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AnalogSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        initLFOShapeData(); // Ensure shape data exists after preset load
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogSynthProcessor();
}
