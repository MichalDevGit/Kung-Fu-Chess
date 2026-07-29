#include "RatingService.h"

#include "EloCalculator.h"
#include "../../persistence/IUserRepository.h"
#include "../../persistence/UserRecord.h"

RatingService::RatingService(IUserRepository& users)
    : users(users)
{
}

void RatingService::applyGameResult(int winnerUserId, int loserUserId)
{
    const std::optional<UserRecord> winner = users.findById(winnerUserId);
    const std::optional<UserRecord> loser = users.findById(loserUserId);

    if (!winner.has_value() || !loser.has_value())
        return;

    const EloCalculator::Outcome outcome = EloCalculator::applyResult(winner->score, loser->score);

    users.setScore(winnerUserId, outcome.newWinnerRating);
    users.setScore(loserUserId, outcome.newLoserRating);
}
