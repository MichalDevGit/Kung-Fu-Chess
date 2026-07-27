#ifndef MONOTONIC_CLOCK_H
#define MONOTONIC_CLOCK_H

#include <chrono>

// Wall-clock milliseconds since an arbitrary, fixed epoch (steady_clock,
// never adjusted by the system clock) -- used wherever server-side code
// needs to timestamp/compare real-time events that happen independently of
// the game engine's own manually-advanced logical clock (RealTimeArbiter):
// matchmaking queue wait time, connection-drop grace periods. Callers only
// ever compare two values from this same function, never interpret one in
// isolation, so the arbitrary epoch is never a problem.
inline long long nowMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

#endif
