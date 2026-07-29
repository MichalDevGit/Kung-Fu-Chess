#include "tests/doctest.h"

#include "services/Rating/EloCalculator.h"

TEST_CASE("Testing EloCalculator")
{
    SUBCASE("equal ratings produce a symmetric change (winner gains exactly what loser loses)")
    {
        const EloCalculator::Outcome outcome = EloCalculator::applyResult(1200, 1200);

        CHECK(outcome.newWinnerRating > 1200);
        CHECK(outcome.newLoserRating < 1200);
        CHECK(outcome.newWinnerRating - 1200 == 1200 - outcome.newLoserRating);
    }

    SUBCASE("a big favorite winning gains only a small amount")
    {
        const EloCalculator::Outcome outcome = EloCalculator::applyResult(1600, 1000);

        const int favoriteGain = outcome.newWinnerRating - 1600;
        CHECK(favoriteGain > 0);
        CHECK(favoriteGain < 5);
    }

    SUBCASE("a big underdog winning gains close to the full K-factor")
    {
        const EloCalculator::Outcome outcome = EloCalculator::applyResult(1000, 1600);

        const int underdogGain = outcome.newWinnerRating - 1000;
        CHECK(underdogGain > 25);
    }

    SUBCASE("the favorite losing drops by close to the full K-factor")
    {
        const EloCalculator::Outcome outcome = EloCalculator::applyResult(1000, 1600);

        const int favoriteLoss = 1600 - outcome.newLoserRating;
        CHECK(favoriteLoss > 25);
    }
}
