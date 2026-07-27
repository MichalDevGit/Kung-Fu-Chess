#ifndef COMMON_CONFIG_MATCHMAKING_CONFIG_H
#define COMMON_CONFIG_MATCHMAKING_CONFIG_H

// Single source of truth for matchmaking/session-resilience tuning, same
// pattern as TimingConfig.h.
namespace MatchmakingConfig
{
    // How long a queued player waits for an opponent before receiving
    // NoMatchResult and being dequeued.
    constexpr long long MAX_WAIT_MILLIS = 60000;

    // Two queued players are considered "approximately the same level" if
    // their score differs by no more than this.
    constexpr int SCORE_RANGE = 200;

    // How long a disconnected participant's opponent waits before the game
    // is forfeited in their favor.
    constexpr long long RECONNECT_GRACE_MILLIS = 30000;
}

#endif
