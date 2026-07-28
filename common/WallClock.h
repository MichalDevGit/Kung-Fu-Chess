#ifndef WALL_CLOCK_H
#define WALL_CLOCK_H

#include <chrono>

// Real (system_clock/Unix-epoch) wall-clock milliseconds -- deliberately a
// different clock source from MonotonicClock.h's nowMillis() (steady_clock).
// steady_clock's epoch is only guaranteed meaningful *within one process*;
// it is not part of the C++ standard's guarantees that two different
// processes (e.g. apigateway/ and server/, possibly in two different Docker
// containers) observe the same steady_clock epoch. common::Security::TokenService
// issues a token in one process and verifies it in another, so it needs a
// clock whose value means the same thing everywhere -- Unix epoch time,
// which is what this function provides. Everything that stays within a
// single process (matchmaking queue wait time, disconnect grace) should
// keep using nowMillis() instead.
inline long long wallClockMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

#endif
