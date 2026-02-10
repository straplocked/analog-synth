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

    osc1Buffer.allocate (samplesPerBlock, true);
    osc2Buffer.allocate (samplesPerBlock, true);
    osc3Buffer.allocate (samplesPerBlock, true);
    subBuffer.allocate (samplesPerBlock, true);
    noiseBuffer.allocate (samplesPerBlock, true);

    // Allocate oversampled buffer
    oversampledBuffer.allocate (samplesPerBlock * kOversampleFactor, true);
    std::fill (firState, firState + kFIRLen, 0.0f);

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

    if (lfoKeySync)
        lfo.resetPhase();
}

void SynthVoice::noteOff()
{
    filterEnv.noteOff();
    ampEnv.noteOff();
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

void SynthVoice::setFilterEnvParams (float attack, float decay, float sustain, float release)
{
    filterEnv.setAttack (attack);
    filterEnv.setDecay (decay);
    filterEnv.setSustain (sustain);
    filterEnv.setRelease (release);
}

void SynthVoice::setAmpEnvParams (float attack, float decay, float sustain, float release)
{
    ampEnv.setAttack (attack);
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

    // Decimate with half-band FIR filter
    for (int i = 0; i < numSamples; ++i)
    {
        // Shift FIR state and insert new samples (take every kOversampleFactor-th output)
        int srcIdx = i * kOversampleFactor;

        // Process both oversampled samples through the FIR
        for (int os = 0; os < kOversampleFactor; ++os)
        {
            // Shift state left
            for (int k = 0; k < kFIRLen - 1; ++k)
                firState[k] = firState[k + 1];
            firState[kFIRLen - 1] = oversampledBuffer[srcIdx + os];
        }

        // Apply FIR and output one sample
        float sum = 0.0f;
        for (int k = 0; k < kFIRLen; ++k)
            sum += firState[k] * halfBandFIR[k];

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

    // Pitch bend multiplier (constant for the block)
    float pbMul = FastMath::exp2 (std::clamp (pitchBendSemitones, -24.0f, 24.0f) / 12.0f);

    // Pitch drift multiplier from analog slop
    float driftMul = FastMath::exp2 (pitchDriftCents / 1200.0f);

    // Unison detune multiplier
    float unisonMul = FastMath::exp2 (unisonDetuneCents / 1200.0f);

    // Phase jitter amount scaled by slop (very subtle: 0.1% of phase increment at max)
    float jitterScale = slopAmount * 0.001f;

    // Per-sample processing at oversampled rate
    for (int i = 0; i < oversampledSamples; ++i)
    {
        // Apply phase jitter for analog VCO simulation
        osc1.setJitterValue (phaseJitter1.processSample() * jitterScale);
        osc2.setJitterValue (phaseJitter2.processSample() * jitterScale);
        osc3.setJitterValue (phaseJitter3.processSample() * jitterScale);

        // Get modulation sources
        float fenvVal = filterEnv.processSample();
        float lfoVal = (hasLFO) ? lfo.processSample() : 0.0f;

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
        // This allows us to process Osc1 (master) first for hard sync
        float osc2ForMod = prevOsc2Out;

        // Apply pitch bend + drift + unison detune + modulation to Osc1 frequency
        float freq1Mod = currentFreq1 * pbMul * driftMul * unisonMul;
        if (hasPolyMod)
        {
            float freqMod = polyMod.fenvToOsc1Freq * fenvVal * 2.0f
                          + polyMod.osc2ToOsc1Freq * osc2ForMod * 0.5f;
            freqMod = std::clamp (freqMod, -10.0f, 10.0f);
            freq1Mod *= FastMath::exp2 (freqMod);
        }
        if (hasLFO && lfoAmounts.toOscFreq != 0.0f)
        {
            freq1Mod *= FastMath::exp2 (std::clamp (lfoAmounts.toOscFreq * lfoVal / 12.0f, -2.0f, 2.0f));
        }
        osc1.setFrequency (std::clamp (freq1Mod, 10.0f, 20000.0f));

        // Apply modulation to Osc1 pulse width
        if (hasPolyMod && (polyMod.fenvToOsc1PW != 0.0f || polyMod.osc2ToOsc1PW != 0.0f))
        {
            float pwMod = basePW1
                        + polyMod.fenvToOsc1PW * fenvVal * 0.4f
                        + polyMod.osc2ToOsc1PW * osc2ForMod * 0.4f;
            osc1.setPulseWidth (std::clamp (pwMod, 0.05f, 0.95f));
        }

        // Generate Osc1 first (master for hard sync)
        float osc1Out = osc1.processSample();

        // Hard sync: if Osc1 wrapped, force Osc2 to reset
        if (syncEnabled && osc1.didPhaseWrap())
            osc2.syncResetAt (osc1.getWrapFraction());

        // Generate Osc2 (slave when sync enabled)
        float freq2Mod = currentFreq2 * pbMul * driftMul * unisonMul;
        if (hasLFO && lfoAmounts.toOscFreq != 0.0f)
            freq2Mod *= FastMath::exp2 (std::clamp (lfoAmounts.toOscFreq * lfoVal / 12.0f, -2.0f, 2.0f));
        osc2.setFrequency (std::clamp (freq2Mod, 10.0f, 20000.0f));
        float osc2Out = osc2.processSample();
        prevOsc2Out = osc2Out;

        // Generate Osc3 (same modulation as Osc2 minus hard sync)
        float freq3Mod = currentFreq3 * pbMul * driftMul * unisonMul;
        if (hasLFO && lfoAmounts.toOscFreq != 0.0f)
            freq3Mod *= FastMath::exp2 (std::clamp (lfoAmounts.toOscFreq * lfoVal / 12.0f, -2.0f, 2.0f));
        osc3.setFrequency (std::clamp (freq3Mod, 10.0f, 20000.0f));
        float osc3Out = osc3.processSample();

        // Sub oscillator: apply pitch bend, drift, unison detune, and LFO modulation
        float subFreq = currentFreq1 * pbMul * driftMul * unisonMul;
        if (hasLFO && lfoAmounts.toOscFreq != 0.0f)
            subFreq *= FastMath::exp2 (std::clamp (lfoAmounts.toOscFreq * lfoVal / 12.0f, -2.0f, 2.0f));
        subOsc.setFrequency (std::clamp (subFreq, 5.0f, 10000.0f));
        float subOut = subOsc.processSample();
        float noiseOut = noise.processSample();

        // Mix with asymmetric saturation for analog warmth
        float rawMix = osc1Out * mixer.getOsc1Level()
                     + osc2Out * mixer.getOsc2Level()
                     + osc3Out * mixer.getOsc3Level()
                     + subOut  * mixer.getSubLevel()
                     + noiseOut * mixer.getNoiseLevel()
                     + dcOffset;
        float mixed = Saturation::warmSaturate (rawMix, 1.0f);

        // Filter modulation (clamp total exponent to safe range)
        float cutoffMod = filterEnvAmount * fenvVal * 4.0f + keyFollowSemitones / 12.0f;

        if (hasPolyMod)
        {
            cutoffMod += polyMod.fenvToLPF * fenvVal * 3.0f;
            cutoffMod += polyMod.osc2ToLPF * osc2Out * 2.0f;
        }
        if (hasLFO && lfoAmounts.toFilter != 0.0f)
            cutoffMod += lfoAmounts.toFilter * lfoVal * 3.0f;

        // Mod wheel → filter (up to 3 octaves)
        if (modWheelAmount > 0.0f)
            cutoffMod += modWheelAmount * 3.0f;

        cutoffMod = std::clamp (cutoffMod, -10.0f, 10.0f);
        float modulatedCutoff = std::clamp (baseCutoff * FastMath::exp2 (cutoffMod), 20.0f, 20000.0f);

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
        hpf.setCutoff (std::clamp (hpfCut, 20.0f, 20000.0f));

        // Apply low-pass filter through FilterEngine
        filterEngine.setCutoff (modulatedCutoff);
        mixed = filterEngine.processSample (mixed);
        mixed = hpf.processSample (mixed);

        // Amp envelope with VCA
        float ampVal = ampEnv.processSample();
        // Exponential VCA response (60dB dynamic range, models OTA-based VCA)
        float ampMod = (ampVal <= 0.0001f) ? 0.0f : FastMath::exp (6.9078f * (ampVal - 1.0f));

        // LFO tremolo
        if (hasLFO && lfoAmounts.toAmp != 0.0f)
            ampMod *= std::max (0.0f, 1.0f + lfoAmounts.toAmp * lfoVal * 0.5f);

        // Analog noise floor (approx -80dB)
        float sample = mixed * ampMod + noiseFloor.processSample() * 0.0001f;

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
