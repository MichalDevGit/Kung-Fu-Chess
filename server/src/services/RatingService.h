#ifndef RATING_SERVICE_H
#define RATING_SERVICE_H

class IUserRepository;

// The single place that turns "who won a game" into "what changed in the
// DB". Both of GameSession's game-ending paths (an ordinary king-capture win
// and a disconnect-timeout forfeit) call the same applyGameResult, so there
// is exactly one rating-update code path in the whole system -- see
// GameSession::forfeitTo and its GameOverEvent subscriber. Depends only on
// IUserRepository, never a concrete backend.
class RatingService
{
public:
    explicit RatingService(IUserRepository& users);

    // Reads both users' current ratings, computes their post-game ratings
    // via EloCalculator, and persists both. No-op (safe) if either user id
    // doesn't resolve to a row -- callers only ever pass ids already known
    // to belong to this session's two participants.
    void applyGameResult(int winnerUserId, int loserUserId);

private:
    IUserRepository& users;
};

#endif
