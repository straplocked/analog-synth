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
#include <cmath>

class PolyBLEPOscillator
{
public:
    enum class Waveform { Saw, Pulse, Triangle, Square };

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        phase = 0.0;
        prevPulseWidth = 0.5f;
        phaseWrapped = false;
        wrapFraction = 0.0;
        syncTriggered = false;
    }

    void setFrequency (float hz)
    {
        phaseIncrement = static_cast<double> (hz) / sampleRate;
    }

    void setWaveform (Waveform w) { waveform = w; }
    void setPulseWidth (float pw) { pulseWidth = juce::jlimit (0.05f, 0.95f, pw); }

    // Phase jitter for analog VCO simulation (set externally per-sample)
    void setJitterValue (float jitter) { jitterValue = jitter; }
    void setPhase (double newPhase) { phase = std::fmod (std::abs (newPhase), 1.0); }

    // Hard sync: check if master wrapped after processSample()
    bool didPhaseWrap() const { return phaseWrapped; }
    double getWrapFraction() const { return wrapFraction; }

    // Hard sync: force slave to reset phase at the fractional sample position
    void syncResetAt (double fraction)
    {
        // Save current waveform value for BLEP discontinuity calculation
        syncOldValue = static_cast<float> (rawSaw());
        // Reset phase to where it would be after the partial sample
        phase = fraction * phaseIncrement;
        syncTriggered = true;
    }

    float processSample()
    {
        float output = 0.0f;
        double dt = phaseIncrement;

        switch (waveform)
        {
            case Waveform::Saw:
            {
                output = static_cast<float> (rawSaw() - polyBLEP (phase, dt));

                // Hard sync BLEP correction: smooth the discontinuity at the reset point
                if (syncTriggered)
                {
                    float newValue = static_cast<float> (rawSaw());
                    float discontinuity = newValue - syncOldValue;
                    // Apply a BLEP proportional to the discontinuity magnitude
                    double syncT = phase / std::max (dt, 1e-10);
                    if (syncT < 1.0)
                        output -= discontinuity * static_cast<float> (syncT - syncT * syncT);
                    syncTriggered = false;
                }
                break;
            }

            case Waveform::Pulse:
            {
                double pw = static_cast<double> (pulseWidth);
                // Use averaged PW for better anti-aliasing of moving edges
                double avgPw = (static_cast<double> (pulseWidth) + static_cast<double> (prevPulseWidth)) * 0.5;

                double saw1 = rawSaw();
                double blep1 = polyBLEP (phase, dt);

                double shiftedPhase = phase + (1.0 - avgPw);
                if (shiftedPhase >= 1.0)
                    shiftedPhase -= 1.0;

                double saw2 = 2.0 * shiftedPhase - 1.0;
                double blep2 = polyBLEP (shiftedPhase, dt);

                output = static_cast<float> ((saw1 - blep1) - (saw2 - blep2));

                if (syncTriggered)
                {
                    float newValue = output;
                    float discontinuity = newValue - syncOldValue;
                    double syncT = phase / std::max (dt, 1e-10);
                    if (syncT < 1.0)
                        output -= discontinuity * static_cast<float> (syncT - syncT * syncT);
                    syncTriggered = false;
                }

                prevPulseWidth = pulseWidth;
                break;
            }

            case Waveform::Square:
            {
                double saw1 = rawSaw();
                double blep1 = polyBLEP (phase, dt);

                double shiftedPhase = phase + 0.5;
                if (shiftedPhase >= 1.0)
                    shiftedPhase -= 1.0;

                double saw2 = 2.0 * shiftedPhase - 1.0;
                double blep2 = polyBLEP (shiftedPhase, dt);

                output = static_cast<float> ((saw1 - blep1) - (saw2 - blep2));

                if (syncTriggered)
                {
                    float newValue = output;
                    float discontinuity = newValue - syncOldValue;
                    double syncT = phase / std::max (dt, 1e-10);
                    if (syncT < 1.0)
                        output -= discontinuity * static_cast<float> (syncT - syncT * syncT);
                    syncTriggered = false;
                }
                break;
            }

            case Waveform::Triangle:
            {
                // Direct PolyBLAMP triangle: naive triangle + BLAMP corrections
                // Naive triangle: rises from -1 to +1 over [0, 0.5], falls over [0.5, 1]
                double naiveTri;
                if (phase < 0.5)
                    naiveTri = 4.0 * phase - 1.0;
                else
                    naiveTri = 3.0 - 4.0 * phase;

                // PolyBLAMP corrections at the two slope discontinuities
                // At phase=0 (trough): slope changes from -4 to +4
                // At phase=0.5 (peak): slope changes from +4 to -4
                double blampScale = 4.0 * dt;
                naiveTri += blampScale * polyBLAMP (phase, dt);

                double shiftedPhase = phase + 0.5;
                if (shiftedPhase >= 1.0)
                    shiftedPhase -= 1.0;
                naiveTri -= blampScale * polyBLAMP (shiftedPhase, dt);

                output = static_cast<float> (naiveTri);
                syncTriggered = false;
                break;
            }
        }

        advancePhase();
        return output;
    }

    void renderBlock (float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    Waveform waveform = Waveform::Saw;
    float pulseWidth = 0.5f;
    float prevPulseWidth = 0.5f;

    // Phase jitter (VCO analog imperfection)
    float jitterValue = 0.0f;

    // Hard sync state
    bool phaseWrapped = false;
    double wrapFraction = 0.0;
    bool syncTriggered = false;
    float syncOldValue = 0.0f;

    double rawSaw() const
    {
        return 2.0 * phase - 1.0;
    }

    void advancePhase()
    {
        // Apply phase jitter for analog VCO simulation
        double inc = phaseIncrement * (1.0 + static_cast<double> (jitterValue));
        phase += inc;

        if (phase >= 1.0)
        {
            phaseWrapped = true;
            // Calculate the fractional position where the wrap occurred
            // (how far into this sample interval the wrap happened)
            wrapFraction = (phase - 1.0) / std::max (inc, 1e-10);
            phase -= 1.0;
        }
        else
        {
            phaseWrapped = false;
            wrapFraction = 0.0;
        }

        // Safety for negative phase from jitter
        if (phase < 0.0)
            phase += 1.0;
    }

    static double polyBLEP (double t, double dt)
    {
        // Discontinuity at t == 0 (equivalently t == 1)
        if (t < dt)
        {
            double x = t / dt;
            return x + x - x * x - 1.0;
        }
        else if (t > 1.0 - dt)
        {
            double x = (t - 1.0) / dt;
            return x * x + x + x + 1.0;
        }
        return 0.0;
    }

    // PolyBLAMP: integral of polyBLEP, corrects 1st-order (ramp) discontinuities
    static double polyBLAMP (double t, double dt)
    {
        if (t < dt)
        {
            double x = t / dt;
            double x2 = x * x;
            return x2 - x2 * x / 3.0 - x;
        }
        else if (t > 1.0 - dt)
        {
            double x = (t - 1.0) / dt;
            double x2 = x * x;
            return x2 * x / 3.0 + x2 + x;
        }
        return 0.0;
    }

};
