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

#include "VoiceManager.h"
#include "../ParameterIDs.h"
#include "../Utility/DebugLog.h"

static float syncDivToHz (int divIndex, double bpm)
{
    // Maps sync division index to beat count
    // 0=1/32, 1=1/16T, 2=1/16, 3=1/8T, 4=1/8, 5=1/4T, 6=1/4, 7=1/2, 8=1Bar, 9=2Bars, 10=4Bars
    static constexpr float beatCounts[] = {
        0.125f,           // 1/32
        1.0f / 6.0f,     // 1/16T (triplet)
        0.25f,            // 1/16
        1.0f / 3.0f,     // 1/8T (triplet)
        0.5f,             // 1/8
        2.0f / 3.0f,     // 1/4T (triplet)
        1.0f,             // 1/4
        2.0f,             // 1/2
        4.0f,             // 1 Bar
        8.0f,             // 2 Bars
        16.0f             // 4 Bars
    };

    int idx = std::clamp (divIndex, 0, 10);
    float beats = beatCounts[idx];
    float seconds = beats * 60.0f / static_cast<float> (bpm);
    return 1.0f / seconds;
}

void VoiceManager::prepare (double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;

    for (int i = 0; i < kMaxVoices; ++i)
    {
        voices[i].prepare (sampleRate, samplesPerBlock);
        voices[i].setVoiceIndex (static_cast<uint32_t> (i));
        drift[i].prepare (sampleRate, static_cast<uint32_t> (i));
        voiceAge[i] = 0;
    }

    chaosOsc.prepare (sampleRate);

    voiceBuffer.allocate (samplesPerBlock, true);
    nextAge = 0;
    heldNotes.clear();
}

// ---- Note routing ----

void VoiceManager::noteOn (int midiNote, float velocity)
{
    if (unisonEnabled)
        noteOnUnison (midiNote, velocity);
    else if (monoMode)
        noteOnMono (midiNote, velocity);
    else
        noteOnPoly (midiNote, velocity);
}

void VoiceManager::noteOff (int midiNote)
{
    if (unisonEnabled)
        noteOffUnison (midiNote);
    else if (monoMode)
        noteOffMono (midiNote);
    else
        noteOffPoly (midiNote);
}

void VoiceManager::allNotesOff()
{
    for (int i = 0; i < kMaxVoices; ++i)
        voices[i].noteOff();
    heldNotes.clear();
}

// ---- Poly mode ----

void VoiceManager::noteOnPoly (int midiNote, float velocity)
{
    int existing = findVoicePlaying (midiNote);
    if (existing >= 0)
    {
        SYNTH_LOG_VOICE ("POLY retrigger: note=" + juce::String (midiNote) + " voice=" + juce::String (existing));
        voices[existing].noteOn (midiNote, velocity);
        voiceAge[existing] = nextAge++;
        return;
    }

    int voiceIdx = findFreeVoice();
    if (voiceIdx < 0)
    {
        voiceIdx = findVoiceToSteal();
        SYNTH_LOG_VOICE ("POLY steal: note=" + juce::String (midiNote) + " voice=" + juce::String (voiceIdx));
    }
    else
    {
        SYNTH_LOG_VOICE ("POLY noteOn: note=" + juce::String (midiNote) + " voice=" + juce::String (voiceIdx));
    }

    voices[voiceIdx].noteOn (midiNote, velocity);
    voiceAge[voiceIdx] = nextAge++;
}

void VoiceManager::noteOffPoly (int midiNote)
{
    int voiceIdx = findVoicePlaying (midiNote);
    if (voiceIdx >= 0)
    {
        SYNTH_LOG_VOICE ("POLY noteOff: note=" + juce::String (midiNote) + " voice=" + juce::String (voiceIdx));
        voices[voiceIdx].noteOff();
    }
}

// ---- Mono mode with legato ----

void VoiceManager::noteOnMono (int midiNote, float velocity)
{
    // True legato: only skip retrigger when notes actually overlap (held notes exist)
    // Using isActive() was wrong — it returns true during release phase, causing
    // notes after noteOff to be treated as legato (no envelope retrigger → silence
    // for zero-sustain patches)
    bool legato = ! heldNotes.empty();
    heldNotes.push_back (midiNote);

    if (legato)
    {
        SYNTH_LOG_VOICE ("MONO legato: note=" + juce::String (midiNote));
        voices[0].setNote (midiNote);
    }
    else
    {
        SYNTH_LOG_VOICE ("MONO noteOn: note=" + juce::String (midiNote) + " vel=" + juce::String (velocity, 2));
        voices[0].noteOn (midiNote, velocity);
    }
}

void VoiceManager::noteOffMono (int midiNote)
{
    // Remove from held notes
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), midiNote), heldNotes.end());

    if (! heldNotes.empty())
    {
        SYNTH_LOG_VOICE ("MONO legato return: note=" + juce::String (heldNotes.back()));
        voices[0].setNote (heldNotes.back());
    }
    else
    {
        SYNTH_LOG_VOICE ("MONO noteOff: note=" + juce::String (midiNote));
        voices[0].noteOff();
    }
}

// ---- Unison mode ----

void VoiceManager::noteOnUnison (int midiNote, float velocity)
{
    // True legato: only skip retrigger when notes actually overlap (held notes exist)
    // Using isActive() was wrong — it returns true during release phase, causing
    // voices to not retrigger envelopes (same bug mono mode had)
    bool legato = ! heldNotes.empty();
    heldNotes.push_back (midiNote);

    SYNTH_LOG_VOICE ("UNISON noteOn: note=" + juce::String (midiNote)
                     + " detune=" + juce::String (unisonDetuneCents, 1) + "ct"
                     + " spread=" + juce::String (unisonSpread, 2)
                     + " legato=" + juce::String (legato ? "yes" : "no"));

    int activeVoices = std::clamp (numUnisonVoices, 2, kMaxVoices);
    int numPairs = activeVoices / 2;

    for (int v = 0; v < kMaxVoices; ++v)
    {
        if (v >= activeVoices)
        {
            // Silence inactive voices
            if (voices[v].isActive())
                voices[v].noteOff();
            voices[v].setUnisonDetune (0.0f);
            continue;
        }

        // Paired detune: each adjacent pair has one voice detuned up, one down.
        int pair = v / 2;
        bool isUpper = (v % 2 == 1);
        float detuneScale = static_cast<float> (pair + 1) / static_cast<float> (numPairs);
        float cents = detuneScale * unisonDetuneCents * (isUpper ? 1.0f : -1.0f);
        voices[v].setUnisonDetune (cents);

        if (legato)
        {
            voices[v].setNote (midiNote);
        }
        else
        {
            voices[v].noteOn (midiNote, velocity);
            // Start in-phase for a strong attack — detune creates chorus naturally
            voices[v].setStartPhase (0.0f);
        }
    }
}

void VoiceManager::noteOffUnison (int midiNote)
{
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), midiNote), heldNotes.end());

    if (! heldNotes.empty())
    {
        int lastNote = heldNotes.back();
        for (int v = 0; v < kMaxVoices; ++v)
            voices[v].setNote (lastNote);
    }
    else
    {
        for (int v = 0; v < kMaxVoices; ++v)
            voices[v].noteOff();
    }
}

// ---- Rendering ----

void VoiceManager::renderBlock (juce::AudioBuffer<float>& outputBuffer, int numSamples)
{
    // Advance analog drift for all voices
    for (int v = 0; v < kMaxVoices; ++v)
        drift[v].advance (numSamples);

    // Process chaos oscillator (global, control-rate — use last sample value)
    for (int i = 0; i < numSamples; ++i)
        globalModValues.chaos = chaosOsc.processSample();

    auto* leftChannel  = outputBuffer.getWritePointer (0);
    auto* rightChannel = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer (1) : nullptr;

    // Clear output
    juce::FloatVectorOperations::clear (leftChannel, numSamples);
    if (rightChannel != nullptr)
        juce::FloatVectorOperations::clear (rightChannel, numSamples);

    if (unisonEnabled && rightChannel != nullptr)
    {
        int activeVoices = std::clamp (numUnisonVoices, 2, kMaxVoices);
        // Gain compensation: 1/sqrt(N) preserves perceived loudness without clipping
        float unisonGain = 1.0f / std::sqrt (static_cast<float> (activeVoices));

        // Unison: per-voice stereo panning with gain compensation
        for (int v = 0; v < kMaxVoices; ++v)
        {
            if (! voices[v].isActive())
                continue;

            voices[v].renderBlock (voiceBuffer, numSamples);

            // Normalized voice position: 0 (first) to 1 (last)
            float t = (activeVoices > 1)
                ? static_cast<float> (v) / static_cast<float> (activeVoices - 1)
                : 0.5f;
            // Pan position: 0 (hard left) to 1 (hard right), centered when spread=0
            float panPos = 0.5f + (t - 0.5f) * unisonSpread;
            float gainL = std::cos (panPos * juce::MathConstants<float>::halfPi) * unisonGain;
            float gainR = std::sin (panPos * juce::MathConstants<float>::halfPi) * unisonGain;

            for (int i = 0; i < numSamples; ++i)
            {
                leftChannel[i]  += voiceBuffer[i] * gainL;
                rightChannel[i] += voiceBuffer[i] * gainR;
            }
        }
    }
    else
    {
        // Poly/Mono: sum to mono, then copy to stereo
        for (int v = 0; v < kMaxVoices; ++v)
        {
            if (! voices[v].isActive())
                continue;

            voices[v].renderBlock (voiceBuffer, numSamples);
            juce::FloatVectorOperations::add (leftChannel, voiceBuffer, numSamples);
        }

        if (rightChannel != nullptr)
            juce::FloatVectorOperations::copy (rightChannel, leftChannel, numSamples);
    }

    // Snapshot envelope levels from most recently triggered active voice
    {
        int newestVoice = -1;
        int newestAge = -1;
        for (int v = 0; v < kMaxVoices; ++v)
        {
            if (voices[v].isActive() && voiceAge[v] > newestAge)
            {
                newestAge = voiceAge[v];
                newestVoice = v;
            }
        }
        if (newestVoice >= 0)
        {
            envLevelFilter.store (voices[newestVoice].getFilterEnvLevel(), std::memory_order_relaxed);
            envLevelAmp.store (voices[newestVoice].getAmpEnvLevel(), std::memory_order_relaxed);
            envLevelEnv3.store (voices[newestVoice].getEnv3Level(), std::memory_order_relaxed);
            envLevelEnv4.store (voices[newestVoice].getEnv4Level(), std::memory_order_relaxed);
        }
        else
        {
            envLevelFilter.store (0.0f, std::memory_order_relaxed);
            envLevelAmp.store (0.0f, std::memory_order_relaxed);
            envLevelEnv3.store (0.0f, std::memory_order_relaxed);
            envLevelEnv4.store (0.0f, std::memory_order_relaxed);
        }
    }

    // Bus saturation: transparent at normal levels, soft-clips when voices stack hot
    // tanh(x/3)*3 is within 1.7% of linear at |x|<1, only engages above ~1.5
    constexpr float kBusHeadroom = 3.0f;
    constexpr float kBusInvHeadroom = 1.0f / kBusHeadroom;
    for (int i = 0; i < numSamples; ++i)
        leftChannel[i] = std::tanh (leftChannel[i] * kBusInvHeadroom) * kBusHeadroom;

    if (rightChannel != nullptr)
    {
        for (int i = 0; i < numSamples; ++i)
            rightChannel[i] = std::tanh (rightChannel[i] * kBusInvHeadroom) * kBusHeadroom;
    }
}

// ---- Parameter update ----

void VoiceManager::updateParameters (juce::AudioProcessorValueTreeState& apvts)
{
    slopAmount = apvts.getRawParameterValue (ParameterIDs::slop)->load();
    monoMode = apvts.getRawParameterValue (ParameterIDs::monoMode)->load() > 0.5f;
    unisonEnabled = apvts.getRawParameterValue (ParameterIDs::unison)->load() > 0.5f;
    unisonDetuneCents = apvts.getRawParameterValue (ParameterIDs::unisonDetune)->load();
    unisonSpread = apvts.getRawParameterValue (ParameterIDs::unisonSpread)->load();
    numUnisonVoices = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::unisonVoices)->load()) + 2; // choice 0=2, 1=3, ..., 4=6

    // ---- Mod Matrix: read 16 slots from APVTS ----
    for (int s = 0; s < ModMatrix::kNumSlots; ++s)
    {
        int srcIdx  = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::modSlotSourceIDs[s])->load());
        int dstIdx  = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::modSlotDestIDs[s])->load());
        float amt   = apvts.getRawParameterValue (ParameterIDs::modSlotAmountIDs[s])->load();
        modMatrix.setSlot (s, srcIdx, dstIdx, amt);
    }
    modMatrix.prepareDestinationMask();

    // ---- Global mod values ----
    globalModValues.modWheel   = modWheelVal;
    globalModValues.aftertouch = aftertouchVal;
    globalModValues.pitchBend  = pitchBendNorm;
    globalModValues.macro1     = apvts.getRawParameterValue (ParameterIDs::macro1)->load();
    globalModValues.macro2     = apvts.getRawParameterValue (ParameterIDs::macro2)->load();
    globalModValues.macro3     = apvts.getRawParameterValue (ParameterIDs::macro3)->load();
    globalModValues.macro4     = apvts.getRawParameterValue (ParameterIDs::macro4)->load();

    // Chaos oscillator params
    chaosOsc.setRate (apvts.getRawParameterValue (ParameterIDs::chaosRate)->load());
    chaosOsc.setAmount (apvts.getRawParameterValue (ParameterIDs::chaosAmount)->load());

    // Pitch bend: normalised value * range in semitones
    float pbRange = apvts.getRawParameterValue (ParameterIDs::pitchBendRange)->load();
    float pbSemitones = pitchBendNorm * pbRange;

    // Glide parameters
    bool glideOn = apvts.getRawParameterValue (ParameterIDs::glide)->load() > 0.5f;
    float glideTimeVal = apvts.getRawParameterValue (ParameterIDs::glideTime)->load();

    // ---- LFO 2/3 parameters (read once, apply to all voices) ----
    int lfo2ShapeVal = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lfo2Shape)->load());
    float lfo2RateVal = apvts.getRawParameterValue (ParameterIDs::lfo2Rate)->load();
    bool lfo2KeySyncVal = apvts.getRawParameterValue (ParameterIDs::lfo2KeySync)->load() > 0.5f;
    bool lfo2SyncOn = apvts.getRawParameterValue (ParameterIDs::lfo2Sync)->load() > 0.5f;
    int lfo2SyncDivVal = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lfo2SyncDiv)->load());
    if (lfo2SyncOn)
        lfo2RateVal = syncDivToHz (lfo2SyncDivVal, currentBpm);

    int lfo3ShapeVal = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lfo3Shape)->load());
    float lfo3RateVal = apvts.getRawParameterValue (ParameterIDs::lfo3Rate)->load();
    bool lfo3KeySyncVal = apvts.getRawParameterValue (ParameterIDs::lfo3KeySync)->load() > 0.5f;
    bool lfo3SyncOn = apvts.getRawParameterValue (ParameterIDs::lfo3Sync)->load() > 0.5f;
    int lfo3SyncDivVal = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lfo3SyncDiv)->load());
    if (lfo3SyncOn)
        lfo3RateVal = syncDivToHz (lfo3SyncDivVal, currentBpm);

    // Step sequencer smoothing per LFO
    bool lfo1StepSmoothVal = apvts.getRawParameterValue (ParameterIDs::lfo1StepSmooth)->load() > 0.5f;
    bool lfo2StepSmoothVal = apvts.getRawParameterValue (ParameterIDs::lfo2StepSmooth)->load() > 0.5f;
    bool lfo3StepSmoothVal = apvts.getRawParameterValue (ParameterIDs::lfo3StepSmooth)->load() > 0.5f;

    // ---- Env 3/4 parameters ----
    float env3A = apvts.getRawParameterValue (ParameterIDs::env3Attack)->load();
    float env3H = apvts.getRawParameterValue (ParameterIDs::env3Hold)->load();
    float env3D = apvts.getRawParameterValue (ParameterIDs::env3Decay)->load();
    float env3S = apvts.getRawParameterValue (ParameterIDs::env3Sustain)->load();
    float env3R = apvts.getRawParameterValue (ParameterIDs::env3Release)->load();

    float env4A = apvts.getRawParameterValue (ParameterIDs::env4Attack)->load();
    float env4H = apvts.getRawParameterValue (ParameterIDs::env4Hold)->load();
    float env4D = apvts.getRawParameterValue (ParameterIDs::env4Decay)->load();
    float env4S = apvts.getRawParameterValue (ParameterIDs::env4Sustain)->load();
    float env4R = apvts.getRawParameterValue (ParameterIDs::env4Release)->load();

    // Envelope loop params
    bool env3LoopOn    = apvts.getRawParameterValue (ParameterIDs::env3LoopEnabled)->load() > 0.5f;
    int env3LoopStart  = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::env3LoopStart)->load());
    int env3LoopEnd    = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::env3LoopEnd)->load());
    bool env4LoopOn    = apvts.getRawParameterValue (ParameterIDs::env4LoopEnabled)->load() > 0.5f;
    int env4LoopStart  = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::env4LoopStart)->load());
    int env4LoopEnd    = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::env4LoopEnd)->load());

    for (int v = 0; v < kMaxVoices; ++v)
    {
        auto& voice = voices[v];

        // Mod matrix + global values (shared across all voices)
        voice.setModMatrix (&modMatrix);
        voice.setGlobalModValues (&globalModValues);

        // Performance controllers
        voice.setPitchBend (pbSemitones);
        voice.setModWheel (modWheelVal);

        // Glide
        voice.setGlideEnabled (glideOn);
        voice.setGlideTime (glideTimeVal);

        // Oscillators
        voice.setOsc1Waveform (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc1Waveform)->load()));
        voice.setOsc2Waveform (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc2Waveform)->load()));
        voice.setOsc3Waveform (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc3Waveform)->load()));
        voice.setOsc1PulseWidth (apvts.getRawParameterValue (ParameterIDs::osc1PulseWidth)->load());
        voice.setOsc2PulseWidth (apvts.getRawParameterValue (ParameterIDs::osc2PulseWidth)->load());
        voice.setOsc3PulseWidth (apvts.getRawParameterValue (ParameterIDs::osc3PulseWidth)->load());
        voice.setSyncEnabled (apvts.getRawParameterValue (ParameterIDs::osc2Sync)->load() > 0.5f);

        voice.setOsc1Tuning (
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc1Octave)->load()),
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc1Semitone)->load()),
            apvts.getRawParameterValue (ParameterIDs::osc1Fine)->load());

        voice.setOsc2Tuning (
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc2Octave)->load()),
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc2Semitone)->load()),
            apvts.getRawParameterValue (ParameterIDs::osc2Fine)->load());

        voice.setOsc3Tuning (
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc3Octave)->load()),
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc3Semitone)->load()),
            apvts.getRawParameterValue (ParameterIDs::osc3Fine)->load());

        // Sub oscillator
        voice.setSubWaveform (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::subWaveform)->load()));
        voice.setSubOctave (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::subOctave)->load()));

        voice.setMixerLevels (
            apvts.getRawParameterValue (ParameterIDs::osc1Level)->load(),
            apvts.getRawParameterValue (ParameterIDs::osc2Level)->load(),
            apvts.getRawParameterValue (ParameterIDs::osc3Level)->load(),
            apvts.getRawParameterValue (ParameterIDs::subLevel)->load(),
            apvts.getRawParameterValue (ParameterIDs::noiseLevel)->load());

        voice.setNoiseType (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::noiseType)->load()));

        // Analog drift — apply per-voice variation
        drift[v].setAmount (slopAmount);
        voice.setPitchDriftCents (drift[v].getPitchDriftCents());
        voice.setDCOffset (drift[v].getDCOffset());
        voice.setSlopAmount (slopAmount);

        // Unison detune (only meaningful in unison mode, but harmless otherwise)
        if (! unisonEnabled)
            voice.setUnisonDetune (0.0f);

        // In unison mode, all voices are one "super voice" — force identical LFO phase
        // so filter/amp modulation doesn't make some voices nearly silent.
        // In poly mode, use golden-ratio offsets for natural modulation spread.
        if (unisonEnabled)
            voice.setVoiceIndex (0);
        else
            voice.setVoiceIndex (static_cast<uint32_t> (v));

        // Filter type, slope, and per-voice analog drift offsets
        voice.setFilterType (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lpfType)->load()));
        voice.setFilterSlope (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lpfSlope)->load()));
        float cutoff = apvts.getRawParameterValue (ParameterIDs::lpfCutoff)->load();
        cutoff *= drift[v].getFilterOffsetFactor();

        float res = apvts.getRawParameterValue (ParameterIDs::lpfResonance)->load();
        res *= drift[v].getResonanceOffsetFactor();

        float drv = apvts.getRawParameterValue (ParameterIDs::lpfDrive)->load();
        drv *= drift[v].getDriveOffsetFactor();

        voice.setFilterCutoff (cutoff);
        voice.setFilterResonance (std::clamp (res, 0.0f, 1.0f));
        voice.setFilterEnvAmount (apvts.getRawParameterValue (ParameterIDs::lpfEnvAmount)->load());
        voice.setFilterKeyFollow (apvts.getRawParameterValue (ParameterIDs::lpfKeyFollow)->load());
        voice.setFilterDrive (std::max (1.0f, drv));
        voice.setHPFCutoff (apvts.getRawParameterValue (ParameterIDs::hpfCutoff)->load());

        // Envelopes — apply per-voice time variation
        float envTimeFactor = drift[v].getEnvTimeOffsetFactor();

        voice.setFilterEnvParams (
            apvts.getRawParameterValue (ParameterIDs::filterEnvAttack)->load() * envTimeFactor,
            apvts.getRawParameterValue (ParameterIDs::filterEnvHold)->load(),
            apvts.getRawParameterValue (ParameterIDs::filterEnvDecay)->load() * envTimeFactor,
            apvts.getRawParameterValue (ParameterIDs::filterEnvSustain)->load(),
            apvts.getRawParameterValue (ParameterIDs::filterEnvRelease)->load() * envTimeFactor);

        voice.setAmpEnvParams (
            apvts.getRawParameterValue (ParameterIDs::ampEnvAttack)->load() * envTimeFactor,
            apvts.getRawParameterValue (ParameterIDs::ampEnvHold)->load(),
            apvts.getRawParameterValue (ParameterIDs::ampEnvDecay)->load() * envTimeFactor,
            apvts.getRawParameterValue (ParameterIDs::ampEnvSustain)->load(),
            apvts.getRawParameterValue (ParameterIDs::ampEnvRelease)->load() * envTimeFactor);

        // Env 3/4 — apply per-voice time variation
        voice.setEnv3Params (env3A * envTimeFactor, env3H, env3D * envTimeFactor, env3S, env3R * envTimeFactor);
        voice.setEnv3Loop (env3LoopOn, env3LoopStart, env3LoopEnd);
        voice.setEnv4Params (env4A * envTimeFactor, env4H, env4D * envTimeFactor, env4S, env4R * envTimeFactor);
        voice.setEnv4Loop (env4LoopOn, env4LoopStart, env4LoopEnd);

        // LFO 1 — apply per-voice rate variation + tempo sync
        voice.setLFOShape (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lfoShape)->load()));
        float lfoRate = apvts.getRawParameterValue (ParameterIDs::lfoRate)->load();
        bool lfo1SyncOn = apvts.getRawParameterValue (ParameterIDs::lfoSync)->load() > 0.5f;
        if (lfo1SyncOn)
            lfoRate = syncDivToHz (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lfoSyncDiv)->load()), currentBpm);
        lfoRate *= drift[v].getLFORateOffsetFactor();
        voice.setLFORate (lfoRate);
        voice.setLFOKeySync (apvts.getRawParameterValue (ParameterIDs::lfoKeySync)->load() > 0.5f);
        voice.setLFOStepSmooth (lfo1StepSmoothVal);

        LFOAmounts lfoAmt;
        lfoAmt.toOscFreq  = apvts.getRawParameterValue (ParameterIDs::lfoToOscFreq)->load();
        lfoAmt.toOscShape = apvts.getRawParameterValue (ParameterIDs::lfoToOscShape)->load();
        lfoAmt.toFilter   = apvts.getRawParameterValue (ParameterIDs::lfoToFilter)->load();
        lfoAmt.toAmp      = apvts.getRawParameterValue (ParameterIDs::lfoToAmp)->load();
        voice.setLFOAmounts (lfoAmt);

        // LFO 2 — apply per-voice rate variation
        voice.setLFO2Shape (lfo2ShapeVal);
        voice.setLFO2Rate (lfo2RateVal * drift[v].getLFORateOffsetFactor());
        voice.setLFO2KeySync (lfo2KeySyncVal);
        voice.setLFO2StepSmooth (lfo2StepSmoothVal);

        // LFO 3 — apply per-voice rate variation
        voice.setLFO3Shape (lfo3ShapeVal);
        voice.setLFO3Rate (lfo3RateVal * drift[v].getLFORateOffsetFactor());
        voice.setLFO3KeySync (lfo3KeySyncVal);
        voice.setLFO3StepSmooth (lfo3StepSmoothVal);

        // LFO shape data (custom shapes + step sequencer) — read from ValueTree
        // Done once outside the loop for efficiency, but applied per-voice here
        // (handled by updateLFOShapeData called after this loop)

        // Poly Mod (Osc2 audio-rate FM — kept as legacy)
        PolyModAmounts pmAmt;
        pmAmt.fenvToOsc1Freq  = apvts.getRawParameterValue (ParameterIDs::polyModFenvToOsc1Freq)->load();
        pmAmt.fenvToOsc1Shape = apvts.getRawParameterValue (ParameterIDs::polyModFenvToOsc1Shape)->load();
        pmAmt.fenvToOsc1PW    = apvts.getRawParameterValue (ParameterIDs::polyModFenvToOsc1PW)->load();
        pmAmt.fenvToLPF       = apvts.getRawParameterValue (ParameterIDs::polyModFenvToLPF)->load();
        pmAmt.fenvToHPF       = apvts.getRawParameterValue (ParameterIDs::polyModFenvToHPF)->load();
        pmAmt.osc2ToOsc1Freq  = apvts.getRawParameterValue (ParameterIDs::polyModOsc2ToOsc1Freq)->load();
        pmAmt.osc2ToOsc1Shape = apvts.getRawParameterValue (ParameterIDs::polyModOsc2ToOsc1Shape)->load();
        pmAmt.osc2ToOsc1PW    = apvts.getRawParameterValue (ParameterIDs::polyModOsc2ToOsc1PW)->load();
        pmAmt.osc2ToLPF       = apvts.getRawParameterValue (ParameterIDs::polyModOsc2ToLPF)->load();
        pmAmt.osc2ToHPF       = apvts.getRawParameterValue (ParameterIDs::polyModOsc2ToHPF)->load();
        voice.setPolyModAmounts (pmAmt);
    }

    // Read custom LFO shape data from ValueTree and push to all voices
    updateLFOShapeData (apvts);
}

void VoiceManager::updateLFOShapeData (juce::AudioProcessorValueTreeState& apvts)
{
    auto shapeData = apvts.state.getChildWithName ("LFO_SHAPE_DATA");
    if (! shapeData.isValid()) return;

    // Helper lambda to read and apply custom + step seq data for one LFO
    for (int lfoIdx = 0; lfoIdx < 3; ++lfoIdx)
    {
        juce::String prefix = "LFO" + juce::String (lfoIdx);

        // Custom shape
        auto customTree = shapeData.getChildWithName (juce::Identifier (prefix + "_CUSTOM"));
        if (customTree.isValid())
        {
            int numPts = static_cast<int> (customTree.getProperty ("numPoints", 2));
            for (int v = 0; v < kMaxVoices; ++v)
            {
                if (lfoIdx == 0)      voices[v].setLFOCustomNumPoints (numPts);
                else if (lfoIdx == 1) voices[v].setLFO2CustomNumPoints (numPts);
                else                  voices[v].setLFO3CustomNumPoints (numPts);

                for (int p = 0; p < numPts; ++p)
                {
                    float x = static_cast<float> (customTree.getProperty (juce::Identifier ("x" + juce::String (p)), 0.0f));
                    float y = static_cast<float> (customTree.getProperty (juce::Identifier ("y" + juce::String (p)), 0.0f));

                    if (lfoIdx == 0)      voices[v].setLFOCustomPoint (p, x, y);
                    else if (lfoIdx == 1) voices[v].setLFO2CustomPoint (p, x, y);
                    else                  voices[v].setLFO3CustomPoint (p, x, y);
                }
            }
        }

        // Step sequencer
        auto stepTree = shapeData.getChildWithName (juce::Identifier (prefix + "_STEPSEQ"));
        if (stepTree.isValid())
        {
            int numSteps = static_cast<int> (stepTree.getProperty ("numSteps", 16));
            for (int v = 0; v < kMaxVoices; ++v)
            {
                if (lfoIdx == 0)      voices[v].setLFOStepSeqNumSteps (numSteps);
                else if (lfoIdx == 1) voices[v].setLFO2StepSeqNumSteps (numSteps);
                else                  voices[v].setLFO3StepSeqNumSteps (numSteps);

                for (int s = 0; s < numSteps; ++s)
                {
                    float val = static_cast<float> (stepTree.getProperty (juce::Identifier ("s" + juce::String (s)), 0.0f));

                    if (lfoIdx == 0)      voices[v].setLFOStepValue (s, val);
                    else if (lfoIdx == 1) voices[v].setLFO2StepValue (s, val);
                    else                  voices[v].setLFO3StepValue (s, val);
                }
            }
        }
    }
}

void VoiceManager::setPitchBend (float normalised)
{
    pitchBendNorm = std::clamp (normalised, -1.0f, 1.0f);
}

void VoiceManager::setModWheel (float value)
{
    modWheelVal = std::clamp (value, 0.0f, 1.0f);
}

void VoiceManager::setAftertouch (float value)
{
    aftertouchVal = std::clamp (value, 0.0f, 1.0f);
}

int VoiceManager::getActiveVoiceCount() const
{
    int count = 0;
    for (int i = 0; i < kMaxVoices; ++i)
        if (voices[i].isActive())
            ++count;
    return count;
}

int VoiceManager::findFreeVoice() const
{
    for (int i = 0; i < kMaxVoices; ++i)
        if (! voices[i].isActive())
            return i;
    return -1;
}

int VoiceManager::findVoiceToSteal() const
{
    int oldest = 0;
    int oldestAge = voiceAge[0];
    for (int i = 1; i < kMaxVoices; ++i)
    {
        if (voiceAge[i] < oldestAge)
        {
            oldestAge = voiceAge[i];
            oldest = i;
        }
    }
    return oldest;
}

int VoiceManager::findVoicePlaying (int midiNote) const
{
    for (int i = 0; i < kMaxVoices; ++i)
        if (voices[i].isActive() && voices[i].getCurrentNote() == midiNote)
            return i;
    return -1;
}
