#ifndef REDIS_MATCH_QUEUE_STORE_H
#define REDIS_MATCH_QUEUE_STORE_H

#include <sw/redis++/redis++.h>

#include "IMatchQueueStore.h"

// Redis-backed IMatchQueueStore. One Redis Hash "matchqueue", field =
// connectionId, value = JSON(Entry). add() uses HSETNX -- atomic and
// idempotent, actually a cleaner primitive match for "no-op if already
// queued" than the local version's contains-then-push.
class RedisMatchQueueStore : public IMatchQueueStore
{
public:
    explicit RedisMatchQueueStore(sw::redis::Redis& redis);

    void add(const Entry& entry) override;
    void remove(const std::string& connectionId) override;
    bool contains(const std::string& connectionId) const override;
    std::vector<Entry> all() const override;

private:
    sw::redis::Redis& redis;
};

#endif
