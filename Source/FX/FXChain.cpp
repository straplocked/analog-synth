#include "FXChain.h"
#include "../ParameterIDs.h"

void FXChain::updateParameters (juce::AudioProcessorValueTreeState& apvts)
{
    // Read FX order parameter and decode permutation
    int orderIndex = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::fxOrder)->load());
    processingOrder = FXOrderUtils::permutationFromIndex (orderIndex);

    // Let each effect read its own parameters
    for (auto& fx : effects)
        fx->updateParameters (apvts);
}
