#include "tests/doctest.h"
#include "services/Sharding/ShardHealthMonitor.h"
#include "services/Sharding/IShardLoadStore.h"
#include "services/Sharding/LocalGameShardRoutingStore.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
// A hand-rolled IShardLoadStore double, not LocalShardLoadStore -- this
// test needs to simulate a lapsed heartbeat (markDead), which
// LocalShardLoadStore deliberately can't do (see its own header comment):
// there's no real cross-process TTL to fake within one process, only
// RedisShardLoadStore has a genuine one.
class FakeShardLoadStore : public IShardLoadStore
{
public:
    void registerShard(const std::string& shardId) override
    {
        load[shardId] = 0;
        alive.insert(shardId);
    }

    void incrementLoad(const std::string& shardId) override { ++load[shardId]; }

    void decrementLoad(const std::string& shardId) override
    {
        auto it = load.find(shardId);
        if (it != load.end() && it->second > 0)
            --it->second;
    }

    std::vector<std::string> knownShards() const override
    {
        std::vector<std::string> shards;
        for (const auto& entry : load)
            shards.push_back(entry.first);
        return shards;
    }

    long long loadOf(const std::string& shardId) const override
    {
        const auto it = load.find(shardId);
        return it != load.end() ? it->second : 0;
    }

    void heartbeat(const std::string& shardId) override { alive.insert(shardId); }
    bool isAlive(const std::string& shardId) const override { return alive.count(shardId) > 0; }

    void forget(const std::string& shardId) override
    {
        load.erase(shardId);
        alive.erase(shardId);
    }

    void markDead(const std::string& shardId) { alive.erase(shardId); }

private:
    std::unordered_map<std::string, long long> load;
    std::unordered_set<std::string> alive;
};

using RecordedPush = std::pair<std::string, std::string>; // connectionId, json
}

TEST_CASE("ShardHealthMonitor leaves every session on a live shard untouched")
{
    FakeShardLoadStore loadStore;
    LocalGameShardRoutingStore routingStore;
    std::vector<RecordedPush> pushes;

    loadStore.registerShard("shard-1");
    routingStore.bindSession("session-1", {1, 2}, "shard-1");

    ShardHealthMonitor monitor(
        loadStore,
        routingStore,
        [](int userId) -> std::optional<std::string> { return "conn-" + std::to_string(userId); },
        [&](const std::string& connectionId, const std::string& json) { pushes.emplace_back(connectionId, json); });

    monitor.checkAndForfeitDeadShards();

    CHECK(pushes.empty());
    CHECK(*routingStore.findShardForSession("session-1") == "shard-1");
    CHECK(loadStore.isAlive("shard-1"));
}

TEST_CASE("ShardHealthMonitor forfeits every session on a dead shard and forgets it")
{
    FakeShardLoadStore loadStore;
    LocalGameShardRoutingStore routingStore;
    std::vector<RecordedPush> pushes;

    loadStore.registerShard("shard-1");
    routingStore.bindSession("session-1", {1, 2}, "shard-1");
    routingStore.bindSession("session-2", {3, 4}, "shard-1");
    loadStore.markDead("shard-1");

    ShardHealthMonitor monitor(
        loadStore,
        routingStore,
        [](int userId) -> std::optional<std::string> { return "conn-" + std::to_string(userId); },
        [&](const std::string& connectionId, const std::string& json) { pushes.emplace_back(connectionId, json); });

    monitor.checkAndForfeitDeadShards();

    REQUIRE(pushes.size() == 4);
    for (const RecordedPush& push : pushes)
    {
        CHECK(push.second.find("\"type\":\"game_over\"") != std::string::npos);
        CHECK(push.second.find("\"reason\":\"shard_unavailable\"") != std::string::npos);
        // No winner -- a shard crash is nobody's fault (see ShardHealthMonitor's class comment).
        CHECK(push.second.find("winnerUserId") == std::string::npos);
    }

    CHECK_FALSE(routingStore.findShardForSession("session-1").has_value());
    CHECK_FALSE(routingStore.findShardForSession("session-2").has_value());

    const std::vector<std::string> remainingShards = loadStore.knownShards();
    CHECK(std::find(remainingShards.begin(), remainingShards.end(), "shard-1") == remainingShards.end());
}

TEST_CASE("ShardHealthMonitor skips a participant who has no resolvable connection right now")
{
    FakeShardLoadStore loadStore;
    LocalGameShardRoutingStore routingStore;
    std::vector<RecordedPush> pushes;

    loadStore.registerShard("shard-1");
    routingStore.bindSession("session-1", {1, 2}, "shard-1");
    loadStore.markDead("shard-1");

    ShardHealthMonitor monitor(
        loadStore,
        routingStore,
        [](int userId) -> std::optional<std::string>
        {
            if (userId == 1)
                return "conn-1";
            return std::nullopt; // userId 2 isn't currently connected
        },
        [&](const std::string& connectionId, const std::string& json) { pushes.emplace_back(connectionId, json); });

    monitor.checkAndForfeitDeadShards();

    REQUIRE(pushes.size() == 1);
    CHECK(pushes[0].first == "conn-1");
}

TEST_CASE("ShardHealthMonitor: a shard with no sessions is still forgotten once dead")
{
    FakeShardLoadStore loadStore;
    LocalGameShardRoutingStore routingStore;
    std::vector<RecordedPush> pushes;

    loadStore.registerShard("shard-1");
    loadStore.markDead("shard-1");

    ShardHealthMonitor monitor(
        loadStore,
        routingStore,
        [](int) -> std::optional<std::string> { return std::nullopt; },
        [&](const std::string& connectionId, const std::string& json) { pushes.emplace_back(connectionId, json); });

    monitor.checkAndForfeitDeadShards();

    CHECK(pushes.empty());
    CHECK(loadStore.knownShards().empty());
}
