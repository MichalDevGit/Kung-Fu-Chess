#ifndef COMMON_CONFIG_RATING_CONFIG_H
#define COMMON_CONFIG_RATING_CONFIG_H

// Single source of truth for ELO-style rating tuning, same pattern as
// TimingConfig.h/MatchmakingConfig.h.
namespace RatingConfig
{
    // Score every newly created user starts at (server/src/persistence/
    // UserRepository::createUser).
    constexpr int INITIAL_RATING = 1200;

    // Standard ELO K-factor: how many rating points are at stake per game.
    // Used by server/src/services/EloCalculator for every game-over outcome,
    // replacing the old flat MatchmakingConfig::SCORE_DELTA_WIN/_LOSS.
    constexpr int K_FACTOR = 32;
}

#endif
