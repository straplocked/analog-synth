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

#include <juce_core/juce_core.h>

namespace ParameterIDs
{
    // Oscillator 1
    inline constexpr auto osc1Waveform   = "osc1_waveform";
    inline constexpr auto osc1Octave     = "osc1_octave";
    inline constexpr auto osc1Semitone   = "osc1_semitone";
    inline constexpr auto osc1Fine       = "osc1_fine";
    inline constexpr auto osc1PulseWidth = "osc1_pulseWidth";
    inline constexpr auto osc1Level      = "osc1_level";

    // Oscillator 2
    inline constexpr auto osc2Waveform   = "osc2_waveform";
    inline constexpr auto osc2Octave     = "osc2_octave";
    inline constexpr auto osc2Semitone   = "osc2_semitone";
    inline constexpr auto osc2Fine       = "osc2_fine";
    inline constexpr auto osc2PulseWidth = "osc2_pulseWidth";
    inline constexpr auto osc2Level      = "osc2_level";
    inline constexpr auto osc2Sync       = "osc2_sync";

    // Oscillator 3
    inline constexpr auto osc3Waveform   = "osc3_waveform";
    inline constexpr auto osc3Octave     = "osc3_octave";
    inline constexpr auto osc3Semitone   = "osc3_semitone";
    inline constexpr auto osc3Fine       = "osc3_fine";
    inline constexpr auto osc3PulseWidth = "osc3_pulseWidth";
    inline constexpr auto osc3Level      = "osc3_level";

    // Sub Oscillator
    inline constexpr auto subLevel    = "sub_level";
    inline constexpr auto subWaveform = "sub_waveform";
    inline constexpr auto subOctave   = "sub_octave";

    // Noise
    inline constexpr auto noiseLevel = "noise_level";
    inline constexpr auto noiseType  = "noise_type";

    // Low-Pass Filter
    inline constexpr auto lpfType      = "lpf_type";
    inline constexpr auto lpfSlope     = "lpf_slope";
    inline constexpr auto lpfCutoff    = "lpf_cutoff";
    inline constexpr auto lpfResonance = "lpf_resonance";
    inline constexpr auto lpfEnvAmount = "lpf_envAmount";
    inline constexpr auto lpfKeyFollow = "lpf_keyFollow";
    inline constexpr auto lpfDrive     = "lpf_drive";

    // High-Pass Filter
    inline constexpr auto hpfCutoff = "hpf_cutoff";

    // Filter Envelope
    inline constexpr auto filterEnvAttack   = "filterEnv_attack";
    inline constexpr auto filterEnvHold     = "filterEnv_hold";
    inline constexpr auto filterEnvDecay    = "filterEnv_decay";
    inline constexpr auto filterEnvSustain  = "filterEnv_sustain";
    inline constexpr auto filterEnvRelease  = "filterEnv_release";
    inline constexpr auto filterEnvVelocity = "filterEnv_velocity";

    // Amp Envelope
    inline constexpr auto ampEnvAttack   = "ampEnv_attack";
    inline constexpr auto ampEnvHold     = "ampEnv_hold";
    inline constexpr auto ampEnvDecay    = "ampEnv_decay";
    inline constexpr auto ampEnvSustain  = "ampEnv_sustain";
    inline constexpr auto ampEnvRelease  = "ampEnv_release";
    inline constexpr auto ampEnvVelocity = "ampEnv_velocity";

    // LFO
    inline constexpr auto lfoShape     = "lfo_shape";
    inline constexpr auto lfoRate      = "lfo_rate";
    inline constexpr auto lfoSync      = "lfo_sync";
    inline constexpr auto lfoSyncDiv   = "lfo_syncDiv";
    inline constexpr auto lfoKeySync   = "lfo_keySync";
    inline constexpr auto lfoToOscFreq  = "lfo_toOscFreq";
    inline constexpr auto lfoToOscShape = "lfo_toOscShape";
    inline constexpr auto lfoToFilter   = "lfo_toFilter";
    inline constexpr auto lfoToAmp      = "lfo_toAmp";

    // Poly Mod — Filter Env sources
    inline constexpr auto polyModFenvToOsc1Freq  = "polyMod_fenvToOsc1Freq";
    inline constexpr auto polyModFenvToOsc1Shape = "polyMod_fenvToOsc1Shape";
    inline constexpr auto polyModFenvToOsc1PW    = "polyMod_fenvToOsc1PW";
    inline constexpr auto polyModFenvToLPF       = "polyMod_fenvToLPF";
    inline constexpr auto polyModFenvToHPF       = "polyMod_fenvToHPF";

    // Poly Mod — Osc2 sources
    inline constexpr auto polyModOsc2ToOsc1Freq  = "polyMod_osc2ToOsc1Freq";
    inline constexpr auto polyModOsc2ToOsc1Shape = "polyMod_osc2ToOsc1Shape";
    inline constexpr auto polyModOsc2ToOsc1PW    = "polyMod_osc2ToOsc1PW";
    inline constexpr auto polyModOsc2ToLPF       = "polyMod_osc2ToLPF";
    inline constexpr auto polyModOsc2ToHPF       = "polyMod_osc2ToHPF";

    // Effects — Distortion
    inline constexpr auto distEnabled = "dist_enabled";
    inline constexpr auto distType    = "dist_type";
    inline constexpr auto distDrive   = "dist_drive";
    inline constexpr auto distTone    = "dist_tone";
    inline constexpr auto distMix     = "dist_mix";

    // Effects — Compressor
    inline constexpr auto compEnabled   = "comp_enabled";
    inline constexpr auto compThreshold = "comp_threshold";
    inline constexpr auto compRatio     = "comp_ratio";
    inline constexpr auto compAttack    = "comp_attack";
    inline constexpr auto compRelease   = "comp_release";
    inline constexpr auto compMakeup    = "comp_makeup";
    inline constexpr auto compMix       = "comp_mix";

    // Effects — Parametric EQ
    inline constexpr auto eqEnabled   = "eq_enabled";
    inline constexpr auto eqLowFreq   = "eq_lowFreq";
    inline constexpr auto eqLowGain   = "eq_lowGain";
    inline constexpr auto eqMidFreq   = "eq_midFreq";
    inline constexpr auto eqMidGain   = "eq_midGain";
    inline constexpr auto eqMidQ      = "eq_midQ";
    inline constexpr auto eqHighFreq  = "eq_highFreq";
    inline constexpr auto eqHighGain  = "eq_highGain";

    // Effects — Chorus
    inline constexpr auto chorusEnabled = "chorus_enabled";
    inline constexpr auto chorusRate    = "chorus_rate";
    inline constexpr auto chorusDepth   = "chorus_depth";
    inline constexpr auto chorusMix     = "chorus_mix";
    inline constexpr auto chorusFlanger = "chorus_flanger";

    // Effects — Delay
    inline constexpr auto delayEnabled  = "delay_enabled";
    inline constexpr auto delayTime     = "delay_time";
    inline constexpr auto delayFeedback = "delay_feedback";
    inline constexpr auto delayMix      = "delay_mix";
    inline constexpr auto delaySync     = "delay_sync";
    inline constexpr auto delaySyncDiv  = "delay_syncDiv";
    inline constexpr auto delayPingPong = "delay_pingPong";
    inline constexpr auto delayBBD      = "delay_bbd";
    inline constexpr auto delayWidth    = "delay_width";

    // Effects — Reverb
    inline constexpr auto reverbEnabled  = "reverb_enabled";
    inline constexpr auto reverbType     = "reverb_type";
    inline constexpr auto reverbSize     = "reverb_size";
    inline constexpr auto reverbDamping  = "reverb_damping";
    inline constexpr auto reverbPreDelay = "reverb_preDelay";
    inline constexpr auto reverbMix      = "reverb_mix";

    // Effects — Chain Order (legacy factoradic — kept for preset migration)
    inline constexpr auto fxOrder = "fx_order";

    // Effects — Slot-based ordering (9 slots, each value = effect index 0-8)
    inline constexpr auto fxOrderSlot0 = "fxOrder_slot0";
    inline constexpr auto fxOrderSlot1 = "fxOrder_slot1";
    inline constexpr auto fxOrderSlot2 = "fxOrder_slot2";
    inline constexpr auto fxOrderSlot3 = "fxOrder_slot3";
    inline constexpr auto fxOrderSlot4 = "fxOrder_slot4";
    inline constexpr auto fxOrderSlot5 = "fxOrder_slot5";
    inline constexpr auto fxOrderSlot6 = "fxOrder_slot6";
    inline constexpr auto fxOrderSlot7 = "fxOrder_slot7";
    inline constexpr auto fxOrderSlot8 = "fxOrder_slot8";

    // Helper array for indexed access to FX slot parameter IDs
    inline const char* const fxOrderSlotIDs[] = {
        fxOrderSlot0, fxOrderSlot1, fxOrderSlot2, fxOrderSlot3,
        fxOrderSlot4, fxOrderSlot5, fxOrderSlot6, fxOrderSlot7,
        fxOrderSlot8
    };

    // Effects — Phaser
    inline constexpr auto phaserEnabled  = "phaser_enabled";
    inline constexpr auto phaserRate     = "phaser_rate";
    inline constexpr auto phaserDepth    = "phaser_depth";
    inline constexpr auto phaserFeedback = "phaser_feedback";
    inline constexpr auto phaserStages   = "phaser_stages";
    inline constexpr auto phaserMix      = "phaser_mix";

    // Effects — Flanger
    inline constexpr auto flangerEnabled  = "flanger_enabled";
    inline constexpr auto flangerRate     = "flanger_rate";
    inline constexpr auto flangerDepth    = "flanger_depth";
    inline constexpr auto flangerFeedback = "flanger_feedback";
    inline constexpr auto flangerMix      = "flanger_mix";

    // Effects — Hyper/Dimension
    inline constexpr auto hyperEnabled = "hyper_enabled";
    inline constexpr auto hyperVoices  = "hyper_voices";
    inline constexpr auto hyperDetune  = "hyper_detune";
    inline constexpr auto hyperSpread  = "hyper_spread";
    inline constexpr auto hyperMix     = "hyper_mix";

    // Global
    inline constexpr auto masterVol       = "global_masterVol";
    inline constexpr auto monoMode        = "global_monoMode";
    inline constexpr auto unison          = "global_unison";
    inline constexpr auto unisonDetune    = "global_unisonDetune";
    inline constexpr auto unisonVoices    = "global_unisonVoices";
    inline constexpr auto unisonSpread    = "global_unisonSpread";
    inline constexpr auto slop            = "global_slop";
    inline constexpr auto glide           = "global_glide";
    inline constexpr auto glideTime       = "global_glideTime";
    inline constexpr auto pitchBendRange  = "global_pitchBendRange";
    inline constexpr auto masterTune      = "global_masterTune";

    // LFO 2
    inline constexpr auto lfo2Shape   = "lfo2_shape";
    inline constexpr auto lfo2Rate    = "lfo2_rate";
    inline constexpr auto lfo2Sync    = "lfo2_sync";
    inline constexpr auto lfo2SyncDiv = "lfo2_syncDiv";
    inline constexpr auto lfo2KeySync = "lfo2_keySync";

    // LFO 3
    inline constexpr auto lfo3Shape   = "lfo3_shape";
    inline constexpr auto lfo3Rate    = "lfo3_rate";
    inline constexpr auto lfo3Sync    = "lfo3_sync";
    inline constexpr auto lfo3SyncDiv = "lfo3_syncDiv";
    inline constexpr auto lfo3KeySync = "lfo3_keySync";

    // Envelope Loop (Env3/Env4 only)
    inline constexpr auto env3LoopEnabled = "env3_loopEnabled";
    inline constexpr auto env3LoopStart   = "env3_loopStart";
    inline constexpr auto env3LoopEnd     = "env3_loopEnd";
    inline constexpr auto env4LoopEnabled = "env4_loopEnabled";
    inline constexpr auto env4LoopStart   = "env4_loopStart";
    inline constexpr auto env4LoopEnd     = "env4_loopEnd";

    // LFO Step Sequencer Smooth
    inline constexpr auto lfo1StepSmooth = "lfo1_stepSmooth";
    inline constexpr auto lfo2StepSmooth = "lfo2_stepSmooth";
    inline constexpr auto lfo3StepSmooth = "lfo3_stepSmooth";

    // Envelope 3
    inline constexpr auto env3Attack   = "env3_attack";
    inline constexpr auto env3Hold     = "env3_hold";
    inline constexpr auto env3Decay    = "env3_decay";
    inline constexpr auto env3Sustain  = "env3_sustain";
    inline constexpr auto env3Release  = "env3_release";
    inline constexpr auto env3Velocity = "env3_velocity";

    // Envelope 4
    inline constexpr auto env4Attack   = "env4_attack";
    inline constexpr auto env4Hold     = "env4_hold";
    inline constexpr auto env4Decay    = "env4_decay";
    inline constexpr auto env4Sustain  = "env4_sustain";
    inline constexpr auto env4Release  = "env4_release";
    inline constexpr auto env4Velocity = "env4_velocity";

    // Macros
    inline constexpr auto macro1 = "macro1";
    inline constexpr auto macro2 = "macro2";
    inline constexpr auto macro3 = "macro3";
    inline constexpr auto macro4 = "macro4";

    // Mod Matrix Slots (16 slots x 3 params)
    inline constexpr auto modSlot0Source  = "modSlot0_source";
    inline constexpr auto modSlot0Dest    = "modSlot0_dest";
    inline constexpr auto modSlot0Amount  = "modSlot0_amount";
    inline constexpr auto modSlot1Source  = "modSlot1_source";
    inline constexpr auto modSlot1Dest    = "modSlot1_dest";
    inline constexpr auto modSlot1Amount  = "modSlot1_amount";
    inline constexpr auto modSlot2Source  = "modSlot2_source";
    inline constexpr auto modSlot2Dest    = "modSlot2_dest";
    inline constexpr auto modSlot2Amount  = "modSlot2_amount";
    inline constexpr auto modSlot3Source  = "modSlot3_source";
    inline constexpr auto modSlot3Dest    = "modSlot3_dest";
    inline constexpr auto modSlot3Amount  = "modSlot3_amount";
    inline constexpr auto modSlot4Source  = "modSlot4_source";
    inline constexpr auto modSlot4Dest    = "modSlot4_dest";
    inline constexpr auto modSlot4Amount  = "modSlot4_amount";
    inline constexpr auto modSlot5Source  = "modSlot5_source";
    inline constexpr auto modSlot5Dest    = "modSlot5_dest";
    inline constexpr auto modSlot5Amount  = "modSlot5_amount";
    inline constexpr auto modSlot6Source  = "modSlot6_source";
    inline constexpr auto modSlot6Dest    = "modSlot6_dest";
    inline constexpr auto modSlot6Amount  = "modSlot6_amount";
    inline constexpr auto modSlot7Source  = "modSlot7_source";
    inline constexpr auto modSlot7Dest    = "modSlot7_dest";
    inline constexpr auto modSlot7Amount  = "modSlot7_amount";
    inline constexpr auto modSlot8Source  = "modSlot8_source";
    inline constexpr auto modSlot8Dest    = "modSlot8_dest";
    inline constexpr auto modSlot8Amount  = "modSlot8_amount";
    inline constexpr auto modSlot9Source  = "modSlot9_source";
    inline constexpr auto modSlot9Dest    = "modSlot9_dest";
    inline constexpr auto modSlot9Amount  = "modSlot9_amount";
    inline constexpr auto modSlot10Source = "modSlot10_source";
    inline constexpr auto modSlot10Dest   = "modSlot10_dest";
    inline constexpr auto modSlot10Amount = "modSlot10_amount";
    inline constexpr auto modSlot11Source = "modSlot11_source";
    inline constexpr auto modSlot11Dest   = "modSlot11_dest";
    inline constexpr auto modSlot11Amount = "modSlot11_amount";
    inline constexpr auto modSlot12Source = "modSlot12_source";
    inline constexpr auto modSlot12Dest   = "modSlot12_dest";
    inline constexpr auto modSlot12Amount = "modSlot12_amount";
    inline constexpr auto modSlot13Source = "modSlot13_source";
    inline constexpr auto modSlot13Dest   = "modSlot13_dest";
    inline constexpr auto modSlot13Amount = "modSlot13_amount";
    inline constexpr auto modSlot14Source = "modSlot14_source";
    inline constexpr auto modSlot14Dest   = "modSlot14_dest";
    inline constexpr auto modSlot14Amount = "modSlot14_amount";
    inline constexpr auto modSlot15Source = "modSlot15_source";
    inline constexpr auto modSlot15Dest   = "modSlot15_dest";
    inline constexpr auto modSlot15Amount = "modSlot15_amount";

    // Helper arrays for indexed access to mod slot parameter IDs
    inline const char* const modSlotSourceIDs[] = {
        modSlot0Source, modSlot1Source, modSlot2Source, modSlot3Source,
        modSlot4Source, modSlot5Source, modSlot6Source, modSlot7Source,
        modSlot8Source, modSlot9Source, modSlot10Source, modSlot11Source,
        modSlot12Source, modSlot13Source, modSlot14Source, modSlot15Source
    };
    inline const char* const modSlotDestIDs[] = {
        modSlot0Dest, modSlot1Dest, modSlot2Dest, modSlot3Dest,
        modSlot4Dest, modSlot5Dest, modSlot6Dest, modSlot7Dest,
        modSlot8Dest, modSlot9Dest, modSlot10Dest, modSlot11Dest,
        modSlot12Dest, modSlot13Dest, modSlot14Dest, modSlot15Dest
    };
    inline const char* const modSlotAmountIDs[] = {
        modSlot0Amount, modSlot1Amount, modSlot2Amount, modSlot3Amount,
        modSlot4Amount, modSlot5Amount, modSlot6Amount, modSlot7Amount,
        modSlot8Amount, modSlot9Amount, modSlot10Amount, modSlot11Amount,
        modSlot12Amount, modSlot13Amount, modSlot14Amount, modSlot15Amount
    };

    // Chaos Oscillator
    inline constexpr auto chaosRate   = "chaos_rate";
    inline constexpr auto chaosAmount = "chaos_amount";

    // Arpeggiator
    inline constexpr auto arpEnabled  = "arp_enabled";
    inline constexpr auto arpMode     = "arp_mode";
    inline constexpr auto arpSyncDiv  = "arp_syncDiv";
    inline constexpr auto arpOctaves  = "arp_octaves";
    inline constexpr auto arpGate     = "arp_gate";
    inline constexpr auto arpLatch          = "arp_latch";
    inline constexpr auto arpStandaloneBpm  = "arp_standaloneBpm";
}
