#include "LocalShardLoadStore.h"

void LocalShardLoadStore::registerShard(const std::string& shardId)
{
    std::lock_guard<std::mutex> lock(mutex);

    loadByShard.emplace(shardId, 0);
}

void LocalShardLoadStore::incrementLoad(const std::string& shardId)
{
    std::lock_guard<std::mutex> lock(mutex);

    ++loadByShard[shardId];
}

void LocalShardLoadStore::decrementLoad(const std::string& shardId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = loadByShard.find(shardId);
    if (it != loadByShard.end() && it->second > 0)
        --it->second;
}

std::vector<std::string> LocalShardLoadStore::knownShards() const
{
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<std::string> shards;
    shards.reserve(loadByShard.size());
    for (const auto& entry : loadByShard)
        shards.push_back(entry.first);

    return shards;
}

long long LocalShardLoadStore::loadOf(const std::string& shardId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = loadByShard.find(shardId);
    return it != loadByShard.end() ? it->second : 0;
}

void LocalShardLoadStore::heartbeat(const std::string& /*shardId*/)
{
    // No-op -- see the class comment for why this is trivially correct here.
}

bool LocalShardLoadStore::isAlive(const std::string& shardId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    return loadByShard.find(shardId) != loadByShard.end();
}

void LocalShardLoadStore::forget(const std::string& shardId)
{
    std::lock_guard<std::mutex> lock(mutex);

    loadByShard.erase(shardId);
}
