#include "catch_amalgamated.hpp"
#include "FX/FXOrderUtils.h"
#include <set>

TEST_CASE ("FXOrderUtils::permutationFromIndex", "[fx]")
{
    SECTION ("Index 0 produces identity permutation")
    {
        auto perm = FXOrderUtils::permutationFromIndex (0);
        for (int i = 0; i < FXOrderUtils::kNumEffects; ++i)
            REQUIRE (perm[static_cast<std::size_t> (i)] == i);
    }

    SECTION ("Index 719 produces reversed permutation")
    {
        auto perm = FXOrderUtils::permutationFromIndex (719);
        for (int i = 0; i < FXOrderUtils::kNumEffects; ++i)
            REQUIRE (perm[static_cast<std::size_t> (i)] == 5 - i);
    }

    SECTION ("Each permutation contains all elements 0-5 exactly once")
    {
        for (int idx = 0; idx < 720; idx += 47) // spot check
        {
            auto perm = FXOrderUtils::permutationFromIndex (idx);
            std::set<int> elements (perm.begin(), perm.end());
            REQUIRE (elements.size() == 6);
            REQUIRE (*elements.begin() == 0);
            REQUIRE (*elements.rbegin() == 5);
        }
    }

    SECTION ("Negative index clamps to 0")
    {
        auto perm = FXOrderUtils::permutationFromIndex (-1);
        auto identity = FXOrderUtils::permutationFromIndex (0);
        REQUIRE (perm == identity);
    }

    SECTION ("Index >= 720 clamps to 719")
    {
        auto perm = FXOrderUtils::permutationFromIndex (720);
        auto last = FXOrderUtils::permutationFromIndex (719);
        REQUIRE (perm == last);
    }
}

TEST_CASE ("FXOrderUtils::indexFromPermutation", "[fx]")
{
    SECTION ("Identity permutation produces index 0")
    {
        std::array<int, 6> identity = { 0, 1, 2, 3, 4, 5 };
        REQUIRE (FXOrderUtils::indexFromPermutation (identity) == 0);
    }

    SECTION ("Reversed permutation produces index 719")
    {
        std::array<int, 6> reversed = { 5, 4, 3, 2, 1, 0 };
        REQUIRE (FXOrderUtils::indexFromPermutation (reversed) == 719);
    }
}

TEST_CASE ("FXOrderUtils roundtrip", "[fx]")
{
    SECTION ("All 720 indices survive roundtrip")
    {
        for (int idx = 0; idx < 720; ++idx)
        {
            auto perm = FXOrderUtils::permutationFromIndex (idx);
            int result = FXOrderUtils::indexFromPermutation (perm);
            REQUIRE (result == idx);
        }
    }

    SECTION ("All 720 permutations are unique")
    {
        std::set<std::array<int, 6>> allPerms;
        for (int idx = 0; idx < 720; ++idx)
            allPerms.insert (FXOrderUtils::permutationFromIndex (idx));
        REQUIRE (allPerms.size() == 720);
    }
}
