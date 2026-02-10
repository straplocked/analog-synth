#include "FXChain.h"
#include "../ParameterIDs.h"

void FXChain::updateParameters (juce::AudioProcessorValueTreeState& apvts)
{
    // Distortion
    distortion.setEnabled (apvts.getRawParameterValue (ParameterIDs::distEnabled)->load() > 0.5f);
    distortion.setDrive (apvts.getRawParameterValue (ParameterIDs::distDrive)->load());
    distortion.setTone (apvts.getRawParameterValue (ParameterIDs::distTone)->load());
    distortion.setMix (apvts.getRawParameterValue (ParameterIDs::distMix)->load());

    // Chorus
    chorus.setEnabled (apvts.getRawParameterValue (ParameterIDs::chorusEnabled)->load() > 0.5f);
    chorus.setRate (apvts.getRawParameterValue (ParameterIDs::chorusRate)->load());
    chorus.setDepth (apvts.getRawParameterValue (ParameterIDs::chorusDepth)->load());
    chorus.setMix (apvts.getRawParameterValue (ParameterIDs::chorusMix)->load());
    chorus.setFlangerMode (apvts.getRawParameterValue (ParameterIDs::chorusFlanger)->load() > 0.5f);

    // Delay
    delay.setEnabled (apvts.getRawParameterValue (ParameterIDs::delayEnabled)->load() > 0.5f);
    delay.setDelayTime (apvts.getRawParameterValue (ParameterIDs::delayTime)->load());
    delay.setFeedback (apvts.getRawParameterValue (ParameterIDs::delayFeedback)->load());
    delay.setMix (apvts.getRawParameterValue (ParameterIDs::delayMix)->load());
    delay.setPingPong (apvts.getRawParameterValue (ParameterIDs::delayPingPong)->load() > 0.5f);
    delay.setBBD (apvts.getRawParameterValue (ParameterIDs::delayBBD)->load() > 0.5f);

    // Reverb
    reverb.setEnabled (apvts.getRawParameterValue (ParameterIDs::reverbEnabled)->load() > 0.5f);
    reverb.setType (static_cast<int> (apvts.getRawParameterValue (ParameterIDs::reverbType)->load()));
    reverb.setSize (apvts.getRawParameterValue (ParameterIDs::reverbSize)->load());
    reverb.setDamping (apvts.getRawParameterValue (ParameterIDs::reverbDamping)->load());
    reverb.setPreDelay (apvts.getRawParameterValue (ParameterIDs::reverbPreDelay)->load());
    reverb.setMix (apvts.getRawParameterValue (ParameterIDs::reverbMix)->load());
}
