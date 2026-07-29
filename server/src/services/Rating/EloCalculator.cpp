#include "EloCalculator.h"

#include <cmath>

#include "../../../../common/Config/RatingConfig.h"

double EloCalculator::expectedScore(int rating, int opponentRating)
{
    return 1.0 / (1.0 + std::pow(10.0, (opponentRating - rating) / 400.0));
}

int EloCalculator::updatedRating(int rating, double expected, double actualScore)
{
    return static_cast<int>(std::lround(rating + RatingConfig::K_FACTOR * (actualScore - expected)));
}

EloCalculator::Outcome EloCalculator::applyResult(int ratingWinner, int ratingLoser)
{
    const double expectedWinner = expectedScore(ratingWinner, ratingLoser);
    const double expectedLoser = expectedScore(ratingLoser, ratingWinner);

    return Outcome{
        updatedRating(ratingWinner, expectedWinner, 1.0),
        updatedRating(ratingLoser, expectedLoser, 0.0)};
}
