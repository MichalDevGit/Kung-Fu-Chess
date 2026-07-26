#ifndef COMMON_CONFIG_TIMING_CONFIG_H
#define COMMON_CONFIG_TIMING_CONFIG_H

// Single source of truth for the real-time engine's timing constants.
// Previously defined directly as GameEngine's own static constexpr members.
namespace TimingConfig
{
    constexpr long long MILLIS_PER_SQUARE = 1000;
    constexpr long long REST_DURATION_MILLIS = 2000;
    constexpr long long JUMP_REST_DURATION_MILLIS = 1000;

    // How often the server's authoritative tick loop advances every active
    // GameSession's clock (introduced in a later phase; defined here now so
    // this constant has exactly one home from the start).
    constexpr long long SERVER_TICK_INTERVAL_MILLIS = 50;
}

#endif
