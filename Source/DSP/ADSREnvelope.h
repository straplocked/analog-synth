#pragma once

#include <cmath>
#include <algorithm>

class ADSREnvelope
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void setAttack (float seconds)  { attackTime = std::max (0.001f, seconds); }
    void setDecay (float seconds)   { decayTime = std::max (0.001f, seconds); }
    void setSustain (float level)   { sustainLevel = std::clamp (level, 0.0f, 1.0f); }
    void setRelease (float seconds) { releaseTime = std::max (0.001f, seconds); }

    void noteOn (float velocity = 1.0f)
    {
        stage = Stage::Attack;
        velocityScale = velocity;
        // Calculate coefficients for exponential curves
        updateCoefficients();
    }

    void noteOff()
    {
        if (stage != Stage::Idle)
        {
            stage = Stage::Release;
            releaseStartLevel = currentLevel;
            releaseCoeff = calcCoeff (releaseTime, 9.21f);
        }
    }

    bool isActive() const { return stage != Stage::Idle; }
    Stage getStage() const { return stage; }
    float getCurrentLevel() const { return currentLevel; }

    float processSample()
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;

            case Stage::Attack:
            {
                // Exponential rise toward 1.0 (with overshoot target for snappy response)
                currentLevel += attackCoeff * (1.3f - currentLevel);
                if (currentLevel >= 1.0f)
                {
                    currentLevel = 1.0f;
                    stage = Stage::Decay;
                }
                break;
            }

            case Stage::Decay:
            {
                // Exponential fall toward sustain level
                currentLevel += decayCoeff * (sustainLevel - currentLevel);
                if (std::abs (currentLevel - sustainLevel) < 0.0001f)
                {
                    currentLevel = sustainLevel;
                    stage = Stage::Sustain;
                }
                break;
            }

            case Stage::Sustain:
                currentLevel = sustainLevel;
                break;

            case Stage::Release:
            {
                // Exponential fall toward 0
                currentLevel += releaseCoeff * (0.0f - currentLevel);
                if (currentLevel < 0.0001f)
                {
                    currentLevel = 0.0f;
                    stage = Stage::Idle;
                }
                break;
            }
        }

        return currentLevel * velocityScale;
    }

    void reset()
    {
        stage = Stage::Idle;
        currentLevel = 0.0f;
        velocityScale = 1.0f;
    }

private:
    double sampleRate = 44100.0;
    Stage stage = Stage::Idle;

    float attackTime   = 0.005f;
    float decayTime    = 0.3f;
    float sustainLevel = 0.8f;
    float releaseTime  = 0.3f;

    float currentLevel = 0.0f;
    float velocityScale = 1.0f;
    float releaseStartLevel = 0.0f;

    float attackCoeff  = 0.0f;
    float decayCoeff   = 0.0f;
    float releaseCoeff = 0.0f;

    float calcCoeff (float timeInSeconds, float numTimeConstants = 1.0f) const
    {
        // One-pole coefficient for exponential curve
        // numTimeConstants scales how far the curve decays in the given time
        return 1.0f - std::exp (-numTimeConstants / (static_cast<float> (sampleRate) * timeInSeconds));
    }

    void updateCoefficients()
    {
        attackCoeff  = calcCoeff (attackTime);            // 1 tau — overshoot target handles the rest
        decayCoeff   = calcCoeff (decayTime, 4.6f);       // reaches ~-40dB in specified time
        releaseCoeff = calcCoeff (releaseTime, 9.21f);    // reaches -80dB (idle threshold) in specified time
    }
};
