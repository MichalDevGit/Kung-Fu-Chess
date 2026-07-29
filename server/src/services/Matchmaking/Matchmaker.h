#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <functional>
#include <string>
#include <vector>

#include "IMatchQueueStore.h"
#include "MatchmakingTypes.h"

// Pairs up queued, authenticated connections into 1v1 matches by score
// proximity and queue order. No networking types, no game engine -- so it's
// driven by whatever already has a tick (the server's existing tick thread,
// see main.cpp's runTickLoop) rather than owning a thread of its own.
//
// Queue storage lives behind IMatchQueueStore (LocalMatchQueueStore by
// default, RedisMatchQueueStore for a multi-node deployment -- see
// server/src/main.cpp's KUNGFUCHESS_REDIS_URL wiring); the pairing/timeout
// algorithm itself stays here, unchanged, operating on a snapshot from
// store.all() each tick.
class Matchmaker
{
public:
    using Entry = ::Entry;
    using Match = ::Match;

    explicit Matchmaker(IMatchQueueStore& store);

    // No-op if this connectionId is already queued (a duplicate find_game
    // from the same connection shouldn't reset its place in line or create
    // a second entry).
    void enqueue(const Entry& entry);

    // Removes a queued entry for this connection, if any -- called on
    // disconnect while still waiting, or when a fresh login supersedes an
    // older connection for the same user.
    void removeByConnection(const std::string& connectionId);

    bool isQueued(const std::string& connectionId) const;

    // Scans a snapshot of the queue once: pairs the earliest-queued entry
    // with the closest-score entry still in the queue within
    // MatchmakingConfig::SCORE_RANGE (removing both from the store), for
    // every pairing possible this tick; then removes and reports every
    // remaining entry that has waited longer than MatchmakingConfig::
    // MAX_WAIT_MILLIS. A concurrent enqueue() arriving mid-tick (after the
    // snapshot, before this tick's removes finish) is a pre-existing-shape,
    // benign race -- that entry just waits for the next tick, well inside
    // MAX_WAIT_MILLIS.
    void tick(
        long long nowMs,
        const std::function<void(const Match&)>& onMatched,
        const std::function<void(const Entry&)>& onTimedOut);

private:
    IMatchQueueStore& store;
};

#endif
