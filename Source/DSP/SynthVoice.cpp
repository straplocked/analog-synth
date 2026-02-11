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

#include "SynthVoice.h"
#include "FastMath.h"
#include "../Utility/DebugLog.h"

void SynthVoice::prepare (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    maxBlockSize = samplesPerBlock;
    oversampledRate = sampleRate * kOversampleFactor;

    // Prepare all DSP at oversampled rate
    osc1.prepare (oversampledRate);
    osc2.prepare (oversampledRate);
    osc3.prepare (oversampledRate);
    subOsc.prepare (oversampledRate);
    lfo.prepare (oversampledRate);

    filterEnv.prepare (oversampledRate);
    ampEnv.prepare (oversampledRate);
    filterEngine.prepare (oversampledRate);
    hpf.prepare (oversampledRate);

    env3.prepare (oversampledRate);
    env4.prepare (oversampledRate);
    lfo2.prepare (oversampledRate);
    lfo3.prepare (oversampledRate);

    noise.prepare (oversampledRate);

    osc1Buffer.allocate (samplesPerBlock, true);
    osc2Buffer.allocate (samplesPerBlock, true);
    osc3Buffer.allocate (samplesPerBlock, true);
    subBuffer.allocate (samplesPerBlock, true);
    noiseBuffer.allocate (samplesPerBlock, true);

    // Allocate oversampled buffer
    oversampledBuffer.allocate (samplesPerBlock * kOversampleFactor, true);
    std::fill (firState, firState + kFIRBufSize, 0.0f);
    firWritePos = 0;

    // Prepare pink noise generators with unique seeds
    phaseJitter1.prepare (54321);
    phaseJitter2.prepare (98765);
    phaseJitter3.prepare (24680);
    noiseFloor.prepare (13579);
}

void SynthVoice::noteOn (int midiNote, float velocity)
{
    int previousNote = currentNote;
    currentNote = midiNote;
    currentVelocity = velocity;

    updateOscillatorFrequencies();

    if (glideEnabled && previousNote >= 0)
    {
        // Don't snap frequencies — let the smoother glide from current position
        glideCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (oversampledRate) * glideTimeSec));
    }
    else
    {
        // Snap to target immediately
        currentFreq1 = baseFreq1;
        currentFreq2 = baseFreq2;
        currentFreq3 = baseFreq3;
    }

    filterEnv.noteOn (velocity);
    ampEnv.noteOn (velocity);
    env3.noteOn (velocity);
    env4.noteOn (velocity);

    // Generate per-note random value for mod matrix Random source
    noteRandomValue = static_cast<float> (static_cast<int> (currentNote * 7 + static_cast<int> (velocity * 127.0f) * 13) % 200 - 100) / 100.0f;

    if (lfoKeySync)
        lfo.resetPhase();
    if (lfo2KeySync)
        lfo2.resetPhase();
    if (lfo3KeySync)
        lfo3.resetPhase();
}

void SynthVoice::noteOff()
{
    filterEnv.noteOff();
    ampEnv.noteOff();
    env3.noteOff();
    env4.noteOff();
}

void SynthVoice::setOsc1Waveform (int waveformIndex)
{
    osc1.setWaveform (static_cast<PolyBLEPOscillator::Waveform> (waveformIndex));
}

void SynthVoice::setOsc2Waveform (int waveformIndex)
{
    osc2.setWaveform (static_cast<PolyBLEPOscillator::Waveform> (waveformIndex));
}

void SynthVoice::setOsc3Waveform (int waveformIndex)
{
    osc3.setWaveform (static_cast<PolyBLEPOscillator::Waveform> (waveformIndex));
}

void SynthVoice::setOsc1PulseWidth (float pw) { basePW1 = pw; osc1.setPulseWidth (pw); }
void SynthVoice::setOsc2PulseWidth (float pw) { osc2.setPulseWidth (pw); }
void SynthVoice::setOsc3PulseWidth (float pw) { osc3.setPulseWidth (pw); }

void SynthVoice::setOsc1Tuning (int octave, int semitone, float fine)
{
    osc1Octave = octave;
    osc1Semitone = semitone;
    osc1Fine = fine;
    if (currentNote >= 0)
        updateOscillatorFrequencies();
}

void SynthVoice::setOsc2Tuning (int octave, int semitone, float fine)
{
    osc2Octave = octave;
    osc2Semitone = semitone;
    osc2Fine = fine;
    if (currentNote >= 0)
        updateOscillatorFrequencies();
}

void SynthVoice::setOsc3Tuning (int octave, int semitone, float fine)
{
    osc3Octave = octave;
    osc3Semitone = semitone;
    osc3Fine = fine;
    if (currentNote >= 0)
        updateOscillatorFrequencies();
}

void SynthVoice::setSubWaveform (int waveformIndex)
{
    subOsc.setWaveform (static_cast<SubOscillator::Waveform> (waveformIndex));
}

void SynthVoice::setSubOctave (int octaveChoice)
{
    subOsc.setOctave (octaveChoice == 0 ? -1 : -2);
}

void SynthVoice::setMixerLevels (float o1, float o2, float o3, float sub, float n)
{
    mixer.setOsc1Level (o1);
    mixer.setOsc2Level (o2);
    mixer.setOsc3Level (o3);
    mixer.setSubLevel (sub);
    mixer.setNoiseLevel (n);
}

void SynthVoice::setFilterCutoff (float hz)        { baseCutoff = hz; }
void SynthVoice::setFilterResonance (float res)     { filterResonance = res; filterEngine.setResonance (res); }
void SynthVoice::setFilterEnvAmount (float amount)  { filterEnvAmount = amount; }
void SynthVoice::setFilterKeyFollow (float amount)  { filterKeyFollow = amount; }
void SynthVoice::setFilterDrive (float driveAmount) { filterDrive = driveAmount; filterEngine.setDrive (driveAmount); }
void SynthVoice::setHPFCutoff (float hz)            { baseHpfCutoff = hz; hpf.setCutoff (hz); }

void SynthVoice::setFilterEnvParams (float attack, float hold, float decay, float sustain, float release)
{
    filterEnv.setAttack (attack);
    filterEnv.setHold (hold);
    filterEnv.setDecay (decay);
    filterEnv.setSustain (sustain);
    filterEnv.setRelease (release);
}

void SynthVoice::setAmpEnvParams (float attack, float hold, float decay, float sustain, float release)
{
    ampEnv.setAttack (attack);
    ampEnv.setHold (hold);
    ampEnv.setDecay (decay);
    ampEnv.setSustain (sustain);
    ampEnv.setRelease (release);
}

void SynthVoice::setGlideTime (float seconds)
{
    glideTimeSec = std::max (0.001f, seconds);
    glideCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (oversampledRate) * glideTimeSec));
}

void SynthVoice::setNote (int midiNote)
{
    // Change pitch without retriggering envelopes (for legato)
    currentNote = midiNote;
    updateOscillatorFrequencies();
    // Glide will handle smooth transition from currentFreq to baseFreq
    if (glideEnabled)
        glideCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (oversampledRate) * glideTimeSec));
    else
    {
        currentFreq1 = baseFreq1;
        currentFreq2 = baseFreq2;
        currentFreq3 = baseFreq3;
    }
}

void SynthVoice::setLFOShape (int shapeIndex)       { lfo.setShape (shapeIndex); }
void SynthVoice::setLFORate (float hz)               { lfo.setRate (hz); }
void SynthVoice::setLFOKeySync (bool enabled)        { lfoKeySync = enabled; lfo.setKeySync (enabled); }
void SynthVoice::setLFOAmounts (const LFOAmounts& a) { lfoAmounts = a; }

void SynthVoice::setLFO2Shape (int shapeIndex)       { lfo2.setShape (shapeIndex); }
void SynthVoice::setLFO2Rate (float hz)               { lfo2.setRate (hz); }
void SynthVoice::setLFO2KeySync (bool enabled)        { lfo2KeySync = enabled; lfo2.setKeySync (enabled); }

void SynthVoice::setLFO3Shape (int shapeIndex)       { lfo3.setShape (shapeIndex); }
void SynthVoice::setLFO3Rate (float hz)               { lfo3.setRate (hz); }
void SynthVoice::setLFO3KeySync (bool enabled)        { lfo3KeySync = enabled; lfo3.setKeySync (enabled); }

void SynthVoice::setEnv3Params (float attack, float hold, float decay, float sustain, float release)
{
    env3.setAttack (attack);
    env3.setHold (hold);
    env3.setDecay (decay);
    env3.setSustain (sustain);
    env3.setRelease (release);
}

void SynthVoice::setEnv4Params (float attack, float hold, float decay, float sustain, float release)
{
    env4.setAttack (attack);
    env4.setHold (hold);
    env4.setDecay (decay);
    env4.setSustain (sustain);
    env4.setRelease (release);
}

void SynthVoice::setPolyModAmounts (const PolyModAmounts& a) { polyMod = a; }

void SynthVoice::renderBlock (float* output, int numSamples)
{
    if (! ampEnv.isActive())
    {
        juce::FloatVectorOperations::clear (output, numSamples);
        return;
    }

    int oversampledSamples = numSamples * kOversampleFactor;

    // Render at oversampled rate into temporary buffer
    renderOversampled (oversampledBuffer, oversampledSamples);

    // Decimate with 15-tap half-band FIR (~43dB stopband, circular buffer)
    for (int i = 0; i < numSamples; ++i)
    {
        int srcIdx = i * kOversampleFactor;

        // Push both oversampled samples into circular buffer
        firState[firWritePos & kFIRBufMask] = oversampledBuffer[srcIdx];
        ++firWritePos;
        firState[firWritePos & kFIRBufMask] = oversampledBuffer[srcIdx + 1];
        ++firWritePos;

        // FIR convolution exploiting half-band symmetry (5 multiply-adds)
        // rp+1 skips the stale slot at firWritePos (16th-oldest, outside our 15-tap window)
        int rp = firWritePos + 1;
        auto at = [&] (int k) -> float { return firState[(rp + k) & kFIRBufMask]; };

        float sum = halfBandFIR[0] * (at (0) + at (14))
                  + halfBandFIR[2] * (at (2) + at (12))
                  + halfBandFIR[4] * (at (4) + at (10))
                  + halfBandFIR[6] * (at (6) + at (8))
                  + halfBandFIR[7] * at (7);

        output[i] = sum;
    }
}

void SynthVoice::renderOversampled (float* output, int oversampledSamples)
{
    // Key follow semitone offset for filter
    float keyFollowSemitones = 0.0f;
    if (currentNote >= 0)
        keyFollowSemitones = filterKeyFollow * static_cast<float> (currentNote - 60);

    bool hasPolyMod = polyMod.isActive();
    bool hasLFO = lfoAmounts.isActive();
    bool hasMod = modMatrix != nullptr && globalModVals != nullptr;

    // Pitch bend multiplier (constant for the block)
    float pbMul = FastMath::exp2 (std::clamp (pitchBendSemitones, -24.0f, 24.0f) / 12.0f);

    // Pitch drift multiplier from analog slop
    float driftMul = FastMath::exp2 (pitchDriftCents / 1200.0f);

    // Unison detune multiplier
    float unisonMul = FastMath::exp2 (unisonDetuneCents / 1200.0f);

    // Phase jitter amount scaled by slop (very subtle: 0.1% of phase increment at max)
    float jitterScale = slopAmount * 0.001f;

    // Pre-compute which mod matrix destinations are active (avoids checking each sample)
    bool matOsc1Freq  = hasMod && modMatrix->hasModulationFor (ModDest::Osc1Freq);
    bool matOsc2Freq  = hasMod && modMatrix->hasModulationFor (ModDest::Osc2Freq);
    bool matOsc3Freq  = hasMod && modMatrix->hasModulationFor (ModDest::Osc3Freq);
    bool matOsc1PW    = hasMod && modMatrix->hasModulationFor (ModDest::Osc1PW);
    bool matOsc2PW    = hasMod && modMatrix->hasModulationFor (ModDest::Osc2PW);
    bool matOsc3PW    = hasMod && modMatrix->hasModulationFor (ModDest::Osc3PW);
    bool matOsc1Level = hasMod && modMatrix->hasModulationFor (ModDest::Osc1Level);
    bool matOsc2Level = hasMod && modMatrix->hasModulationFor (ModDest::Osc2Level);
    bool matOsc3Level = hasMod && modMatrix->hasModulationFor (ModDest::Osc3Level);
    bool matSubLevel  = hasMod && modMatrix->hasModulationFor (ModDest::SubLevel);
    bool matNoiseLevel = hasMod && modMatrix->hasModulationFor (ModDest::NoiseLevel);
    bool matLPFCutoff = hasMod && modMatrix->hasModulationFor (ModDest::LPFCutoff);
    bool matLPFRes    = hasMod && modMatrix->hasModulationFor (ModDest::LPFResonance);
    bool matHPFCutoff = hasMod && modMatrix->hasModulationFor (ModDest::HPFCutoff);
    bool matFilterDrv = hasMod && modMatrix->hasModulationFor (ModDest::FilterDrive);
    bool matFiltEnvAmt = hasMod && modMatrix->hasModulationFor (ModDest::FilterEnvAmt);
    bool matAmpLevel  = hasMod && modMatrix->hasModulationFor (ModDest::AmpLevel);

    // Per-sample processing at oversampled rate
    for (int i = 0; i < oversampledSamples; ++i)
    {
        // Apply phase jitter for analog VCO simulation
        osc1.setJitterValue (phaseJitter1.processSample() * jitterScale);
        osc2.setJitterValue (phaseJitter2.processSample() * jitterScale);
        osc3.setJitterValue (phaseJitter3.processSample() * jitterScale);

        // ---- Compute all modulation sources ----
        float fenvVal = filterEnv.processSample();
        float ampVal  = ampEnv.processSample();
        float env3Val = env3.processSample();
        float env4Val = env4.processSample();
        float lfoVal  = lfo.processSample();
        float lfo2Val = lfo2.processSample();
        float lfo3Val = lfo3.processSample();

        // Build per-voice mod values for this sample
        VoiceModValues voiceModVals;
        if (hasMod)
        {
            voiceModVals.filterEnv  = fenvVal;
            voiceModVals.ampEnv     = ampVal;
            voiceModVals.env3       = env3Val;
            voiceModVals.env4       = env4Val;
            voiceModVals.lfo1       = lfoVal;
            voiceModVals.lfo2       = lfo2Val;
            voiceModVals.lfo3       = lfo3Val;
            voiceModVals.velocity   = currentVelocity;
            voiceModVals.noteNumber = (currentNote >= 0)
                ? static_cast<float> (currentNote - 60) / 60.0f : 0.0f;
            voiceModVals.random     = noteRandomValue;
        }

        // Glide: smoothly move towards target frequency
        if (glideEnabled)
        {
            currentFreq1 += glideCoeff * (baseFreq1 - currentFreq1);
            currentFreq2 += glideCoeff * (baseFreq2 - currentFreq2);
            currentFreq3 += glideCoeff * (baseFreq3 - currentFreq3);
        }
        else
        {
            currentFreq1 = baseFreq1;
            currentFreq2 = baseFreq2;
            currentFreq3 = baseFreq3;
        }

        // Use previous sample's Osc2 output for Poly Mod FM (1-sample delay, inaudible)
        float osc2ForMod = prevOsc2Out;

        // ---- Osc1 frequency modulation ----
        float freq1Mod = currentFreq1 * pbMul * driftMul * unisonMul;
        if (hasPolyMod)
        {
            float freqMod = polyMod.fenvToOsc1Freq * fenvVal * 2.0f
                          + polyMod.osc2ToOsc1Freq * osc2ForMod * 0.5f;
            freqMod = std::clamp (freqMod, -10.0f, 10.0f);
            freq1Mod *= FastMath::exp2 (freqMod);
        }
        if (hasLFO && lfoAmounts.toOscFreq != 0.0f)
            freq1Mod *= FastMath::exp2 (std::clamp (lfoAmounts.toOscFreq * lfoVal / 12.0f, -2.0f, 2.0f));
        if (matOsc1Freq)
        {
            float m = modMatrix->computeModulation (ModDest::Osc1Freq, voiceModVals, *globalModVals);
            freq1Mod *= FastMath::exp2 (std::clamp (m * 2.0f, -4.0f, 4.0f));
        }
        osc1.setFrequency (std::clamp (freq1Mod, 10.0f, 20000.0f));

        // ---- Osc1 pulse width modulation ----
        if (hasPolyMod && (polyMod.fenvToOsc1PW != 0.0f || polyMod.osc2ToOsc1PW != 0.0f))
        {
            float pwMod = basePW1
                        + polyMod.fenvToOsc1PW * fenvVal * 0.4f
                        + polyMod.osc2ToOsc1PW * osc2ForMod * 0.4f;
            osc1.setPulseWidth (std::clamp (pwMod, 0.05f, 0.95f));
        }
        if (matOsc1PW)
        {
            float m = modMatrix->computeModulation (ModDest::Osc1PW, voiceModVals, *globalModVals);
            osc1.setPulseWidth (std::clamp (basePW1 + m * 0.45f, 0.05f, 0.95f));
        }

        // Generate Osc1 first (master for hard sync)
        float osc1Out = osc1.processSample();

        // Hard sync: if Osc1 wrapped, force Osc2 to reset
        if (syncEnabled && osc1.didPhaseWrap())
            osc2.syncResetAt (osc1.getWrapFraction());

        // ---- Osc2 frequency modulation ----
        float freq2Mod = currentFreq2 * pbMul * driftMul * unisonMul;
        if (hasLFO && lfoAmounts.toOscFreq != 0.0f)
            freq2Mod *= FastMath::exp2 (std::clamp (lfoAmounts.toOscFreq * lfoVal / 12.0f, -2.0f, 2.0f));
        if (matOsc2Freq)
        {
            float m = modMatrix->computeModulation (ModDest::Osc2Freq, voiceModVals, *globalModVals);
            freq2Mod *= FastMath::exp2 (std::clamp (m * 2.0f, -4.0f, 4.0f));
        }
        osc2.setFrequency (std::clamp (freq2Mod, 10.0f, 20000.0f));
        if (matOsc2PW)
        {
            float m = modMatrix->computeModulation (ModDest::Osc2PW, voiceModVals, *globalModVals);
            osc2.setPulseWidth (std::clamp (0.5f + m * 0.45f, 0.05f, 0.95f));
        }
        float osc2Out = osc2.processSample();
        prevOsc2Out = osc2Out;

        // ---- Osc3 frequency modulation ----
        float freq3Mod = currentFreq3 * pbMul * driftMul * unisonMul;
        if (hasLFO && lfoAmounts.toOscFreq != 0.0f)
            freq3Mod *= FastMath::exp2 (std::clamp (lfoAmounts.toOscFreq * lfoVal / 12.0f, -2.0f, 2.0f));
        if (matOsc3Freq)
        {
            float m = modMatrix->computeModulation (ModDest::Osc3Freq, voiceModVals, *globalModVals);
            freq3Mod *= FastMath::exp2 (std::clamp (m * 2.0f, -4.0f, 4.0f));
        }
        osc3.setFrequency (std::clamp (freq3Mod, 10.0f, 20000.0f));
        if (matOsc3PW)
        {
            float m = modMatrix->computeModulation (ModDest::Osc3PW, voiceModVals, *globalModVals);
            osc3.setPulseWidth (std::clamp (0.5f + m * 0.45f, 0.05f, 0.95f));
        }
        float osc3Out = osc3.processSample();

        // ---- Sub oscillator ----
        float subFreq = currentFreq1 * pbMul * driftMul * unisonMul;
        if (hasLFO && lfoAmounts.toOscFreq != 0.0f)
            subFreq *= FastMath::exp2 (std::clamp (lfoAmounts.toOscFreq * lfoVal / 12.0f, -2.0f, 2.0f));
        subOsc.setFrequency (std::clamp (subFreq, 5.0f, 10000.0f));
        float subOut = subOsc.processSample();
        float noiseOut = noise.processSample();

        // ---- Mixer with optional level modulation ----
        float lvl1 = mixer.getOsc1Level();
        float lvl2 = mixer.getOsc2Level();
        float lvl3 = mixer.getOsc3Level();
        float lvlS = mixer.getSubLevel();
        float lvlN = mixer.getNoiseLevel();

        if (matOsc1Level)
            lvl1 = std::clamp (lvl1 + modMatrix->computeModulation (ModDest::Osc1Level, voiceModVals, *globalModVals), 0.0f, 1.0f);
        if (matOsc2Level)
            lvl2 = std::clamp (lvl2 + modMatrix->computeModulation (ModDest::Osc2Level, voiceModVals, *globalModVals), 0.0f, 1.0f);
        if (matOsc3Level)
            lvl3 = std::clamp (lvl3 + modMatrix->computeModulation (ModDest::Osc3Level, voiceModVals, *globalModVals), 0.0f, 1.0f);
        if (matSubLevel)
            lvlS = std::clamp (lvlS + modMatrix->computeModulation (ModDest::SubLevel, voiceModVals, *globalModVals), 0.0f, 1.0f);
        if (matNoiseLevel)
            lvlN = std::clamp (lvlN + modMatrix->computeModulation (ModDest::NoiseLevel, voiceModVals, *globalModVals), 0.0f, 1.0f);

        float rawMix = osc1Out * lvl1
                     + osc2Out * lvl2
                     + osc3Out * lvl3
                     + subOut  * lvlS
                     + noiseOut * lvlN
                     + dcOffset;
        float mixed = Saturation::warmSaturate (rawMix, 1.0f);

        // ---- Filter modulation ----
        float envAmt = filterEnvAmount;
        if (matFiltEnvAmt)
            envAmt += modMatrix->computeModulation (ModDest::FilterEnvAmt, voiceModVals, *globalModVals);

        float cutoffMod = envAmt * fenvVal * 4.0f + keyFollowSemitones / 12.0f;

        if (hasPolyMod)
        {
            cutoffMod += polyMod.fenvToLPF * fenvVal * 3.0f;
            cutoffMod += polyMod.osc2ToLPF * osc2Out * 2.0f;
        }
        if (hasLFO && lfoAmounts.toFilter != 0.0f)
            cutoffMod += lfoAmounts.toFilter * lfoVal * 3.0f;

        if (matLPFCutoff)
            cutoffMod += modMatrix->computeModulation (ModDest::LPFCutoff, voiceModVals, *globalModVals) * 4.0f;

        // Mod wheel → filter (up to 3 octaves)
        if (modWheelAmount > 0.0f)
            cutoffMod += modWheelAmount * 3.0f;

        cutoffMod = std::clamp (cutoffMod, -10.0f, 10.0f);
        float modulatedCutoff = std::clamp (baseCutoff * FastMath::exp2 (cutoffMod), 20.0f, 20000.0f);

        // Filter resonance modulation
        if (matLPFRes)
        {
            float resMod = filterResonance + modMatrix->computeModulation (ModDest::LPFResonance, voiceModVals, *globalModVals);
            filterEngine.setResonance (std::clamp (resMod, 0.0f, 1.0f));
        }

        // Filter drive modulation
        if (matFilterDrv)
        {
            float drvMod = filterDrive + modMatrix->computeModulation (ModDest::FilterDrive, voiceModVals, *globalModVals) * 4.0f;
            filterEngine.setDrive (std::max (1.0f, drvMod));
        }

        // HPF modulation
        float hpfCut = baseHpfCutoff;
        if (hasPolyMod && polyMod.fenvToHPF != 0.0f)
        {
            float hpfMod = std::clamp (polyMod.fenvToHPF * fenvVal * 3.0f, -10.0f, 10.0f);
            hpfCut *= FastMath::exp2 (hpfMod);
        }
        if (hasPolyMod && polyMod.osc2ToHPF != 0.0f)
        {
            float hpfMod = std::clamp (polyMod.osc2ToHPF * osc2Out * 2.0f, -10.0f, 10.0f);
            hpfCut *= FastMath::exp2 (hpfMod);
        }
        if (matHPFCutoff)
        {
            float m = modMatrix->computeModulation (ModDest::HPFCutoff, voiceModVals, *globalModVals);
            hpfCut *= FastMath::exp2 (std::clamp (m * 4.0f, -10.0f, 10.0f));
        }
        hpf.setCutoff (std::clamp (hpfCut, 20.0f, 20000.0f));

        // Apply low-pass filter through FilterEngine
        filterEngine.setCutoff (modulatedCutoff);
        mixed = filterEngine.processSample (mixed);
        mixed = hpf.processSample (mixed);

        // ---- Amp envelope with VCA ----
        // Exponential VCA response (60dB dynamic range, models OTA-based VCA)
        constexpr float vcaK = 6.9078f;           // ln(1000) for 60dB range
        constexpr float vcaFloor = 0.001f;        // exp(-vcaK)
        constexpr float vcaScale = 1.0f / (1.0f - vcaFloor);
        float ampMod = (ampVal <= 0.0001f) ? 0.0f
            : (FastMath::exp (vcaK * (ampVal - 1.0f)) - vcaFloor) * vcaScale;

        // LFO tremolo (legacy hardwired)
        if (hasLFO && lfoAmounts.toAmp != 0.0f)
            ampMod *= std::max (0.0f, 1.0f + lfoAmounts.toAmp * lfoVal * 0.5f);

        // Mod matrix amp level modulation
        if (matAmpLevel)
            ampMod *= std::max (0.0f, 1.0f + modMatrix->computeModulation (ModDest::AmpLevel, voiceModVals, *globalModVals));

        // VCA with OTA crossover distortion (models LM13700 dead zone near zero)
        float vcaOut = mixed * ampMod;
        constexpr float crossoverK = 50.0f;
        float x2 = vcaOut * vcaOut;
        float crossoverGain = x2 / (x2 + 1.0f / (crossoverK * crossoverK));
        vcaOut *= crossoverGain;

        // Analog noise floor (approx -80dB)
        float sample = vcaOut + noiseFloor.processSample() * 0.0001f;

        // NaN/Inf safety net: if anything went wrong, output silence
        if (std::isnan (sample) || std::isinf (sample))
        {
            SYNTH_LOG_ERROR ("NaN/Inf in voice output, note=" + juce::String (currentNote) + " resetting filters");
            sample = 0.0f;
            filterEngine.reset();
            hpf.reset();
        }

        output[i] = sample;
    }
}

float SynthVoice::noteToFrequency (int midiNote, int octaveOffset, int semitoneOffset, float centsOffset) const
{
    float noteNum = static_cast<float> (midiNote + octaveOffset * 12 + semitoneOffset) + centsOffset / 100.0f;
    return 440.0f * std::pow (2.0f, (noteNum - 69.0f) / 12.0f);
}

void SynthVoice::updateOscillatorFrequencies()
{
    if (currentNote < 0)
        return;

    baseFreq1 = noteToFrequency (currentNote, osc1Octave, osc1Semitone, osc1Fine);
    baseFreq2 = noteToFrequency (currentNote, osc2Octave, osc2Semitone, osc2Fine);
    baseFreq3 = noteToFrequency (currentNote, osc3Octave, osc3Semitone, osc3Fine);

    osc1.setFrequency (baseFreq1);
    osc2.setFrequency (baseFreq2);
    osc3.setFrequency (baseFreq3);
    subOsc.setFrequency (baseFreq1); // Sub follows Osc1
}
