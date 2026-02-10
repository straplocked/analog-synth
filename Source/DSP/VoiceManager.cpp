#include "VoiceManager.h"
#include "../ParameterIDs.h"

void VoiceManager::prepare (double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;

    for (int i = 0; i < kMaxVoices; ++i)
    {
        voices[i].prepare (sampleRate, samplesPerBlock);
        drift[i].prepare (sampleRate, static_cast<uint32_t> (i));
        voiceAge[i] = 0;
    }

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
        voices[existing].noteOn (midiNote, velocity);
        voiceAge[existing] = nextAge++;
        return;
    }

    int voiceIdx = findFreeVoice();
    if (voiceIdx < 0)
        voiceIdx = findVoiceToSteal();

    voices[voiceIdx].noteOn (midiNote, velocity);
    voiceAge[voiceIdx] = nextAge++;
}

void VoiceManager::noteOffPoly (int midiNote)
{
    int voiceIdx = findVoicePlaying (midiNote);
    if (voiceIdx >= 0)
        voices[voiceIdx].noteOff();
}

// ---- Mono mode with legato ----

void VoiceManager::noteOnMono (int midiNote, float velocity)
{
    heldNotes.push_back (midiNote);
    bool legato = voices[0].isActive();

    if (legato)
        voices[0].setNote (midiNote); // change pitch without retriggering
    else
        voices[0].noteOn (midiNote, velocity);
}

void VoiceManager::noteOffMono (int midiNote)
{
    // Remove from held notes
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), midiNote), heldNotes.end());

    if (! heldNotes.empty())
    {
        // Return to last held note (legato)
        voices[0].setNote (heldNotes.back());
    }
    else
    {
        voices[0].noteOff();
    }
}

// ---- Unison mode ----

void VoiceManager::noteOnUnison (int midiNote, float velocity)
{
    heldNotes.push_back (midiNote);

    for (int v = 0; v < kMaxVoices; ++v)
    {
        // Symmetric detune spread: voice 0 = -max, voice 5 = +max
        float detuneOffset = (static_cast<float> (v) / static_cast<float> (kMaxVoices - 1) - 0.5f) * 2.0f;
        float cents = detuneOffset * unisonDetuneCents;
        voices[v].setUnisonDetune (cents);

        bool legato = voices[v].isActive();
        if (legato)
            voices[v].setNote (midiNote);
        else
            voices[v].noteOn (midiNote, velocity);
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

    auto* leftChannel  = outputBuffer.getWritePointer (0);
    auto* rightChannel = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer (1) : nullptr;

    // Clear output
    juce::FloatVectorOperations::clear (leftChannel, numSamples);
    if (rightChannel != nullptr)
        juce::FloatVectorOperations::clear (rightChannel, numSamples);

    if (unisonEnabled && rightChannel != nullptr)
    {
        // Unison: per-voice stereo panning
        for (int v = 0; v < kMaxVoices; ++v)
        {
            if (! voices[v].isActive())
                continue;

            voices[v].renderBlock (voiceBuffer, numSamples);

            // Pan position: voice 0 = left, voice 5 = right, scaled by spread
            float pan = (static_cast<float> (v) / static_cast<float> (kMaxVoices - 1)) * unisonSpread;
            // Convert to 0-1 range centered when spread=0
            float panPos = 0.5f + (pan - unisonSpread * 0.5f);
            float gainL = std::cos (panPos * static_cast<float> (M_PI) * 0.5f);
            float gainR = std::sin (panPos * static_cast<float> (M_PI) * 0.5f);

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
}

// ---- Parameter update ----

void VoiceManager::updateParameters (juce::AudioProcessorValueTreeState& apvts)
{
    slopAmount = apvts.getRawParameterValue (ParameterIDs::slop)->load();
    monoMode = apvts.getRawParameterValue (ParameterIDs::monoMode)->load() > 0.5f;
    unisonEnabled = apvts.getRawParameterValue (ParameterIDs::unison)->load() > 0.5f;
    unisonDetuneCents = apvts.getRawParameterValue (ParameterIDs::unisonDetune)->load();
    unisonSpread = apvts.getRawParameterValue (ParameterIDs::unisonSpread)->load();

    // Pitch bend: normalised value * range in semitones
    float pbRange = apvts.getRawParameterValue (ParameterIDs::pitchBendRange)->load();
    float pbSemitones = pitchBendNorm * pbRange;

    // Glide parameters
    bool glideOn = apvts.getRawParameterValue (ParameterIDs::glide)->load() > 0.5f;
    float glideTimeVal = apvts.getRawParameterValue (ParameterIDs::glideTime)->load();

    for (int v = 0; v < kMaxVoices; ++v)
    {
        auto& voice = voices[v];

        // Performance controllers
        voice.setPitchBend (pbSemitones);
        voice.setModWheel (modWheelVal);

        // Glide
        voice.setGlideEnabled (glideOn);
        voice.setGlideTime (glideTimeVal);

        // Oscillators
        voice.setOsc1Waveform (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc1Waveform)->load()));
        voice.setOsc2Waveform (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc2Waveform)->load()));
        voice.setOsc1PulseWidth (apvts.getRawParameterValue (ParameterIDs::osc1PulseWidth)->load());
        voice.setOsc2PulseWidth (apvts.getRawParameterValue (ParameterIDs::osc2PulseWidth)->load());
        voice.setSyncEnabled (apvts.getRawParameterValue (ParameterIDs::osc2Sync)->load() > 0.5f);

        voice.setOsc1Tuning (
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc1Octave)->load()),
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc1Semitone)->load()),
            apvts.getRawParameterValue (ParameterIDs::osc1Fine)->load());

        voice.setOsc2Tuning (
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc2Octave)->load()),
            static_cast<int> (apvts.getRawParameterValue (ParameterIDs::osc2Semitone)->load()),
            apvts.getRawParameterValue (ParameterIDs::osc2Fine)->load());

        voice.setMixerLevels (
            apvts.getRawParameterValue (ParameterIDs::osc1Level)->load(),
            apvts.getRawParameterValue (ParameterIDs::osc2Level)->load(),
            apvts.getRawParameterValue (ParameterIDs::subLevel)->load(),
            apvts.getRawParameterValue (ParameterIDs::noiseLevel)->load());

        // Analog drift — apply per-voice variation
        drift[v].setAmount (slopAmount);
        voice.setPitchDriftCents (drift[v].getPitchDriftCents());
        voice.setDCOffset (drift[v].getDCOffset());
        voice.setSlopAmount (slopAmount);

        // Unison detune (only meaningful in unison mode, but harmless otherwise)
        if (! unisonEnabled)
            voice.setUnisonDetune (0.0f);

        // Filter type and per-voice analog drift offsets
        voice.setFilterType (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lpfType)->load()));
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
            apvts.getRawParameterValue (ParameterIDs::filterEnvDecay)->load() * envTimeFactor,
            apvts.getRawParameterValue (ParameterIDs::filterEnvSustain)->load(),
            apvts.getRawParameterValue (ParameterIDs::filterEnvRelease)->load() * envTimeFactor);

        voice.setAmpEnvParams (
            apvts.getRawParameterValue (ParameterIDs::ampEnvAttack)->load() * envTimeFactor,
            apvts.getRawParameterValue (ParameterIDs::ampEnvDecay)->load() * envTimeFactor,
            apvts.getRawParameterValue (ParameterIDs::ampEnvSustain)->load(),
            apvts.getRawParameterValue (ParameterIDs::ampEnvRelease)->load() * envTimeFactor);

        // LFO — apply per-voice rate variation
        voice.setLFOShape (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lfoShape)->load()));
        float lfoRate = apvts.getRawParameterValue (ParameterIDs::lfoRate)->load();
        lfoRate *= drift[v].getLFORateOffsetFactor();
        voice.setLFORate (lfoRate);
        voice.setLFOKeySync (apvts.getRawParameterValue (ParameterIDs::lfoKeySync)->load() > 0.5f);

        LFOAmounts lfoAmt;
        lfoAmt.toOscFreq  = apvts.getRawParameterValue (ParameterIDs::lfoToOscFreq)->load();
        lfoAmt.toOscShape = apvts.getRawParameterValue (ParameterIDs::lfoToOscShape)->load();
        lfoAmt.toFilter   = apvts.getRawParameterValue (ParameterIDs::lfoToFilter)->load();
        lfoAmt.toAmp      = apvts.getRawParameterValue (ParameterIDs::lfoToAmp)->load();
        voice.setLFOAmounts (lfoAmt);

        // Poly Mod
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
}

void VoiceManager::setPitchBend (float normalised)
{
    pitchBendNorm = std::clamp (normalised, -1.0f, 1.0f);
}

void VoiceManager::setModWheel (float value)
{
    modWheelVal = std::clamp (value, 0.0f, 1.0f);
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
