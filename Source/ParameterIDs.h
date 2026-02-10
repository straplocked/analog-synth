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
    inline constexpr auto filterEnvDecay    = "filterEnv_decay";
    inline constexpr auto filterEnvSustain  = "filterEnv_sustain";
    inline constexpr auto filterEnvRelease  = "filterEnv_release";
    inline constexpr auto filterEnvVelocity = "filterEnv_velocity";

    // Amp Envelope
    inline constexpr auto ampEnvAttack   = "ampEnv_attack";
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
    inline constexpr auto distDrive   = "dist_drive";
    inline constexpr auto distTone    = "dist_tone";
    inline constexpr auto distMix     = "dist_mix";

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

    // Effects — Reverb
    inline constexpr auto reverbEnabled  = "reverb_enabled";
    inline constexpr auto reverbType     = "reverb_type";
    inline constexpr auto reverbSize     = "reverb_size";
    inline constexpr auto reverbDamping  = "reverb_damping";
    inline constexpr auto reverbPreDelay = "reverb_preDelay";
    inline constexpr auto reverbMix      = "reverb_mix";

    // Global
    inline constexpr auto masterVol       = "global_masterVol";
    inline constexpr auto monoMode        = "global_monoMode";
    inline constexpr auto unison          = "global_unison";
    inline constexpr auto unisonDetune    = "global_unisonDetune";
    inline constexpr auto unisonSpread    = "global_unisonSpread";
    inline constexpr auto slop            = "global_slop";
    inline constexpr auto glide           = "global_glide";
    inline constexpr auto glideTime       = "global_glideTime";
    inline constexpr auto pitchBendRange  = "global_pitchBendRange";
    inline constexpr auto masterTune      = "global_masterTune";

    // Arpeggiator
    inline constexpr auto arpEnabled  = "arp_enabled";
    inline constexpr auto arpMode     = "arp_mode";
    inline constexpr auto arpSyncDiv  = "arp_syncDiv";
    inline constexpr auto arpOctaves  = "arp_octaves";
    inline constexpr auto arpGate     = "arp_gate";
    inline constexpr auto arpLatch          = "arp_latch";
    inline constexpr auto arpStandaloneBpm  = "arp_standaloneBpm";
}
