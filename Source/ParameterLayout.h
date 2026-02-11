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
#include "ParameterIDs.h"

namespace ParameterLayout
{

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto freqRange = juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f);
    auto timeRange = juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f);
    auto pctRange  = juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f);
    auto biRange   = juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f);

    // ---- Oscillator 1 ----
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::osc1Waveform, 1 }, "Osc1 Waveform",
        juce::StringArray { "Saw", "Pulse", "Triangle", "Square" }, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::osc1Octave, 1 }, "Osc1 Octave", -2, 2, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::osc1Semitone, 1 }, "Osc1 Semitone", -12, 12, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc1Fine, 1 }, "Osc1 Fine",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc1PulseWidth, 1 }, "Osc1 PW",
        juce::NormalisableRange<float> (0.05f, 0.95f, 0.01f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc1Level, 1 }, "Osc1 Level", pctRange, 0.8f));

    // ---- Oscillator 2 ----
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::osc2Waveform, 1 }, "Osc2 Waveform",
        juce::StringArray { "Saw", "Pulse", "Triangle", "Square" }, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::osc2Octave, 1 }, "Osc2 Octave", -2, 2, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::osc2Semitone, 1 }, "Osc2 Semitone", -12, 12, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc2Fine, 1 }, "Osc2 Fine",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc2PulseWidth, 1 }, "Osc2 PW",
        juce::NormalisableRange<float> (0.05f, 0.95f, 0.01f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc2Level, 1 }, "Osc2 Level", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::osc2Sync, 1 }, "Osc2 Sync", false));

    // ---- Oscillator 3 ----
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::osc3Waveform, 1 }, "Osc3 Waveform",
        juce::StringArray { "Saw", "Pulse", "Triangle", "Square" }, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::osc3Octave, 1 }, "Osc3 Octave", -2, 2, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::osc3Semitone, 1 }, "Osc3 Semitone", -12, 12, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc3Fine, 1 }, "Osc3 Fine",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc3PulseWidth, 1 }, "Osc3 PW",
        juce::NormalisableRange<float> (0.05f, 0.95f, 0.01f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::osc3Level, 1 }, "Osc3 Level", pctRange, 0.0f));

    // ---- Sub & Noise ----
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::subLevel, 1 }, "Sub Level", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::subWaveform, 1 }, "Sub Waveform",
        juce::StringArray { "Sine", "Triangle", "Square", "Pulse" }, 1));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::subOctave, 1 }, "Sub Octave",
        juce::StringArray { "-1 Oct", "-2 Oct" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::noiseLevel, 1 }, "Noise Level", pctRange, 0.0f));

    // ---- Low-Pass Filter ----
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::lpfType, 1 }, "LPF Type",
        juce::StringArray { "Ladder", "Diode", "Sequential", "Acid", "MS20" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::lpfSlope, 1 }, "LPF Slope",
        juce::StringArray { "12dB", "24dB" }, 1));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lpfCutoff, 1 }, "LPF Cutoff", freqRange, 8000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lpfResonance, 1 }, "LPF Resonance", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lpfEnvAmount, 1 }, "LPF Env Amount", biRange, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lpfKeyFollow, 1 }, "LPF Key Follow", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lpfDrive, 1 }, "LPF Drive",
        juce::NormalisableRange<float> (1.0f, 5.0f, 0.01f), 1.0f));

    // ---- High-Pass Filter ----
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::hpfCutoff, 1 }, "HPF Cutoff", freqRange, 20.0f));

    // ---- Filter Envelope ----
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::filterEnvAttack, 1 }, "Filter Env Attack", timeRange, 0.01f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::filterEnvDecay, 1 }, "Filter Env Decay", timeRange, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::filterEnvSustain, 1 }, "Filter Env Sustain", pctRange, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::filterEnvRelease, 1 }, "Filter Env Release", timeRange, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::filterEnvVelocity, 1 }, "Filter Env Velocity", pctRange, 0.5f));

    // ---- Amp Envelope ----
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::ampEnvAttack, 1 }, "Amp Env Attack", timeRange, 0.005f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::ampEnvDecay, 1 }, "Amp Env Decay", timeRange, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::ampEnvSustain, 1 }, "Amp Env Sustain", pctRange, 0.8f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::ampEnvRelease, 1 }, "Amp Env Release", timeRange, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::ampEnvVelocity, 1 }, "Amp Env Velocity", pctRange, 0.5f));

    // ---- LFO ----
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::lfoShape, 1 }, "LFO Shape",
        juce::StringArray { "Triangle", "Saw", "Square", "Random", "Sine" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lfoRate, 1 }, "LFO Rate",
        juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 2.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::lfoSync, 1 }, "LFO Sync", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::lfoSyncDiv, 1 }, "LFO Sync Div",
        juce::StringArray { "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 Bar", "2 Bars", "4 Bars" }, 6));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::lfoKeySync, 1 }, "LFO Key Sync", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lfoToOscFreq, 1 }, "LFO to Osc Freq", biRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lfoToOscShape, 1 }, "LFO to Osc Shape", biRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lfoToFilter, 1 }, "LFO to Filter", biRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::lfoToAmp, 1 }, "LFO to Amp", biRange, 0.0f));

    // ---- Poly Mod (Filter Env sources) ----
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModFenvToOsc1Freq, 1 }, "PM FEnv>Osc1 Freq", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModFenvToOsc1Shape, 1 }, "PM FEnv>Osc1 Shape", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModFenvToOsc1PW, 1 }, "PM FEnv>Osc1 PW", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModFenvToLPF, 1 }, "PM FEnv>LPF", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModFenvToHPF, 1 }, "PM FEnv>HPF", pctRange, 0.0f));

    // ---- Poly Mod (Osc2 sources) ----
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModOsc2ToOsc1Freq, 1 }, "PM Osc2>Osc1 Freq", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModOsc2ToOsc1Shape, 1 }, "PM Osc2>Osc1 Shape", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModOsc2ToOsc1PW, 1 }, "PM Osc2>Osc1 PW", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModOsc2ToLPF, 1 }, "PM Osc2>LPF", pctRange, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::polyModOsc2ToHPF, 1 }, "PM Osc2>HPF", pctRange, 0.0f));

    // ---- Distortion ----
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::distEnabled, 1 }, "Distortion On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::distType, 1 }, "Dist Type",
        juce::StringArray { "Tanh", "Tube", "Hard Clip", "Fuzz", "Bitcrush", "Tape", "Wavefolder" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::distDrive, 1 }, "Dist Drive",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.5f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::distTone, 1 }, "Dist Tone", pctRange, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::distMix, 1 }, "Dist Mix", pctRange, 1.0f));

    // ---- Compressor ----
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::compEnabled, 1 }, "Compressor On", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::compThreshold, 1 }, "Comp Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::compRatio, 1 }, "Comp Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.4f), 4.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::compAttack, 1 }, "Comp Attack",
        juce::NormalisableRange<float> (0.1f, 200.0f, 0.1f, 0.3f), 10.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::compRelease, 1 }, "Comp Release",
        juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.3f), 100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::compMakeup, 1 }, "Comp Makeup",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::compMix, 1 }, "Comp Mix", pctRange, 1.0f));

    // ---- Parametric EQ ----
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::eqEnabled, 1 }, "EQ On", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::eqLowFreq, 1 }, "EQ Low Freq",
        juce::NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.3f), 200.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::eqLowGain, 1 }, "EQ Low Gain",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::eqMidFreq, 1 }, "EQ Mid Freq",
        juce::NormalisableRange<float> (100.0f, 10000.0f, 1.0f, 0.3f), 1000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::eqMidGain, 1 }, "EQ Mid Gain",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::eqMidQ, 1 }, "EQ Mid Q",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::eqHighFreq, 1 }, "EQ High Freq",
        juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.3f), 5000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::eqHighGain, 1 }, "EQ High Gain",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f));

    // ---- Chorus ----
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::chorusEnabled, 1 }, "Chorus On", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::chorusRate, 1 }, "Chorus Rate",
        juce::NormalisableRange<float> (0.1f, 5.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::chorusDepth, 1 }, "Chorus Depth", pctRange, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::chorusMix, 1 }, "Chorus Mix", pctRange, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::chorusFlanger, 1 }, "Chorus Flanger", false));

    // ---- Delay ----
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::delayEnabled, 1 }, "Delay On", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::delayTime, 1 }, "Delay Time",
        juce::NormalisableRange<float> (1.0f, 2000.0f, 0.1f, 0.3f), 400.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::delayFeedback, 1 }, "Delay Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.4f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::delayMix, 1 }, "Delay Mix", pctRange, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::delaySync, 1 }, "Delay Sync", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::delaySyncDiv, 1 }, "Delay Sync Div",
        juce::StringArray { "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 Bar" }, 6));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::delayPingPong, 1 }, "Delay Ping Pong", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::delayBBD, 1 }, "Delay BBD", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::delayWidth, 1 }, "Delay Width", pctRange, 1.0f));

    // ---- Reverb ----
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::reverbEnabled, 1 }, "Reverb On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::reverbType, 1 }, "Reverb Type",
        juce::StringArray { "Room", "Hall", "Plate", "Spring" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::reverbSize, 1 }, "Reverb Size", pctRange, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::reverbDamping, 1 }, "Reverb Damping", pctRange, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::reverbPreDelay, 1 }, "Reverb Pre-Delay",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 10.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::reverbMix, 1 }, "Reverb Mix", pctRange, 0.2f));

    // ---- FX Chain Order ----
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::fxOrder, 1 }, "FX Order", 0, 719, 0));

    // ---- Global ----
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::masterVol, 1 }, "Master Volume",
        juce::NormalisableRange<float> (-60.0f, 6.0f, 0.1f, 2.5f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::monoMode, 1 }, "Mono", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::unison, 1 }, "Unison", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::unisonDetune, 1 }, "Unison Detune",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 15.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::unisonSpread, 1 }, "Unison Spread", pctRange, 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::slop, 1 }, "Slop", pctRange, 0.2f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::glide, 1 }, "Glide", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::glideTime, 1 }, "Glide Time",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.3f), 0.1f));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::pitchBendRange, 1 }, "Pitch Bend Range", 1, 24, 2));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::masterTune, 1 }, "Master Tune",
        juce::NormalisableRange<float> (430.0f, 450.0f, 0.1f), 440.0f));

    // ---- Arpeggiator ----
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::arpEnabled, 1 }, "Arp On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::arpMode, 1 }, "Arp Mode",
        juce::StringArray { "Up", "Down", "Up/Down", "Random", "As Played" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::arpSyncDiv, 1 }, "Arp Rate",
        juce::StringArray { "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 Bar" }, 4));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { ParameterIDs::arpOctaves, 1 }, "Arp Octaves", 1, 4, 1));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::arpGate, 1 }, "Arp Gate",
        juce::NormalisableRange<float> (0.1f, 1.0f, 0.01f), 0.75f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::arpLatch, 1 }, "Arp Latch", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::arpStandaloneBpm, 1 }, "Arp BPM",
        juce::NormalisableRange<float> (30.0f, 300.0f, 0.1f), 120.0f));

    return layout;
}

} // namespace ParameterLayout
