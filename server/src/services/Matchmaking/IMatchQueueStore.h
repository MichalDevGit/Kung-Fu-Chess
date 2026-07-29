#ifndef IMATCH_QUEUE_STORE_H
#define IMATCH_QUEUE_STORE_H

#include <string>
#include <vector>

#include "MatchmakingTypes.h"

// Raw storage behind Matchmaker -- the pairing/timeout algorithm itself
// stays in Matchmaker::tick, operating on a snapshot from all(); this
// interface only covers where queued entries live. Mirrors the
// IUserRepository pattern: LocalMatchQueueStore is the in-memory default,
// RedisMatchQueueStore is the Redis-backed alternative.
class IMatchQueueStore
{
public:
    virtual ~IMatchQueueStore() = default;

    // No-op if connectionId is already present -- mirrors Matchmaker::
    // enqueue's "duplicate find_game from the same connection" guard.
    virtual void add(const Entry& entry) = 0;
    virtual void remove(const std::string& connectionId) = 0;
    virtual bool contains(const std::string& connectionId) const = 0;
    virtual std::vector<Entry> all() const = 0;
};

#endif
