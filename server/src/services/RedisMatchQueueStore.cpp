#include "RedisMatchQueueStore.h"

#include <unordered_map>

namespace
{
    const std::string QUEUE_KEY = "matchqueue";
}

RedisMatchQueueStore::RedisMatchQueueStore(sw::redis::Redis& redis) : redis(redis)
{
}

void RedisMatchQueueStore::add(const Entry& entry)
{
    // HSETNX: only sets the field if it doesn't already exist -- the exact
    // "no-op if already queued" semantics Matchmaker::enqueue needs, for
    // free, atomically (no separate contains-then-add round trip needed).
    redis.hsetnx(QUEUE_KEY, entry.connectionId, entry.toJson().dump());
}

void RedisMatchQueueStore::remove(const std::string& connectionId)
{
    redis.hdel(QUEUE_KEY, connectionId);
}

bool RedisMatchQueueStore::contains(const std::string& connectionId) const
{
    return redis.hexists(QUEUE_KEY, connectionId);
}

std::vector<Entry> RedisMatchQueueStore::all() const
{
    std::unordered_map<std::string, std::string> raw;
    redis.hgetall(QUEUE_KEY, std::inserter(raw, raw.begin()));

    std::vector<Entry> entries;
    entries.reserve(raw.size());
    for (const auto& [connectionId, json] : raw)
        entries.push_back(Entry::fromJson(nlohmann::json::parse(json)));

    return entries;
}
