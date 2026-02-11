#include "catch_amalgamated.hpp"
#include "DSP/Mixer.h"
#include <cmath>

TEST_CASE ("Mixer level getters/setters", "[dsp]")
{
    Mixer mixer;

    SECTION ("Default levels")
    {
        REQUIRE (mixer.getOsc1Level() == Catch::Approx (0.8f));
        REQUIRE (mixer.getOsc2Level() == Catch::Approx (0.0f));
        REQUIRE (mixer.getOsc3Level() == Catch::Approx (0.0f));
        REQUIRE (mixer.getSubLevel() == Catch::Approx (0.0f));
        REQUIRE (mixer.getNoiseLevel() == Catch::Approx (0.0f));
    }

    SECTION ("Set and get levels")
    {
        mixer.setOsc1Level (0.5f);
        mixer.setOsc2Level (0.3f);
        mixer.setOsc3Level (0.2f);
        mixer.setSubLevel (0.1f);
        mixer.setNoiseLevel (0.05f);

        REQUIRE (mixer.getOsc1Level() == Catch::Approx (0.5f));
        REQUIRE (mixer.getOsc2Level() == Catch::Approx (0.3f));
        REQUIRE (mixer.getOsc3Level() == Catch::Approx (0.2f));
        REQUIRE (mixer.getSubLevel() == Catch::Approx (0.1f));
        REQUIRE (mixer.getNoiseLevel() == Catch::Approx (0.05f));
    }
}

TEST_CASE ("Mixer::mixBlock", "[dsp]")
{
    Mixer mixer;
    constexpr int N = 64;

    float osc1[N], osc2[N], sub[N], noise[N], output[N];

    SECTION ("Single source at unity produces tanh(source * level)")
    {
        mixer.setOsc1Level (1.0f);
        mixer.setOsc2Level (0.0f);
        mixer.setSubLevel (0.0f);
        mixer.setNoiseLevel (0.0f);

        for (int i = 0; i < N; ++i)
        {
            osc1[i] = 0.5f;
            osc2[i] = 0.0f;
            sub[i] = 0.0f;
            noise[i] = 0.0f;
        }

        mixer.mixBlock (osc1, osc2, sub, noise, output, N);

        for (int i = 0; i < N; ++i)
            REQUIRE (output[i] == Catch::Approx (std::tanh (0.5f)));
    }

    SECTION ("Silence in -> silence out")
    {
        mixer.setOsc1Level (1.0f);
        for (int i = 0; i < N; ++i)
        {
            osc1[i] = 0.0f;
            osc2[i] = 0.0f;
            sub[i] = 0.0f;
            noise[i] = 0.0f;
        }

        mixer.mixBlock (osc1, osc2, sub, noise, output, N);

        for (int i = 0; i < N; ++i)
            REQUIRE (output[i] == Catch::Approx (0.0f));
    }

    SECTION ("Soft clipping bounds output")
    {
        mixer.setOsc1Level (1.0f);
        mixer.setOsc2Level (1.0f);
        mixer.setSubLevel (1.0f);
        mixer.setNoiseLevel (1.0f);

        for (int i = 0; i < N; ++i)
        {
            osc1[i] = 1.0f;
            osc2[i] = 1.0f;
            sub[i] = 1.0f;
            noise[i] = 1.0f;
        }

        mixer.mixBlock (osc1, osc2, sub, noise, output, N);

        for (int i = 0; i < N; ++i)
        {
            REQUIRE (output[i] < 1.0f);
            REQUIRE (output[i] > 0.0f);
        }
    }
}
