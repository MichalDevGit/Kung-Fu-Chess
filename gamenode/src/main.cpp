// Game Server Shard entry point (MIGRATION_PLAN.md Phase 3, reshaped by
// Phase 4b): owns one shard's GameSessionManager and GameSession's tick loop
// only -- Matchmaker/MatchmakingRequestHandler/GameAllocator moved out to
// their own singleton process (gameallocator/, see gameallocator/src/
// main.cpp) in Phase 4b, since they need to exist exactly once no matter how
// many of *this* process run at once. Every push a GameSession used to send
// via server.sendTo now goes out over GameNodeConfig::PUSHES_CHANNEL instead
// (GameNodePushPublisher); every move/jump/connection-close/reconnect-check/
// create-session the WebSocket Gateway (or the Allocator) used to dispatch
// in-process now arrives over this shard's own
// GameNodeConfig::shardRequestsChannel(shardId) instead
// (GameNodeRequestRouter). See ARCHITECTURE.md for the full design.
//
// Unlike server/src/main.cpp (where KUNGFUCHESS_REDIS_URL is still an
// opt-in), Redis is mandatory here: this process's GameSessionManager's
// session-index store MUST be the same Redis every other process is pointed
// at, or they silently diverge on "who's connected"/"which session is
// this" -- the same class of problem MIGRATION_PLAN.md Phase 2 flagged for
// KUNGFUCHESS_POSTGRES_URL, just for Redis now that an actual process split
// exists.
//
// Phase 4b: this process no longer holds ConnectionRegistry, Matchmaker, or
// GameAllocator at all -- find_game and shard-picking are gameallocator/'s
// job now. KUNGFUCHESS_SHARD_ID (default "shard-1") is this shard's own
// identity: it's what it registers with IShardLoadStore, and what
// GameNodeConfig::shardRequestsChannel(shardId) it subscribes to for its own
// requests. Running two replicas with the same (or both defaulted) shard id
// is a real misconfiguration once more than one is ever run at once -- see
// ARCHITECTURE.md's Known gaps.
#include <chrono>
#include <cstdlib>
#include <future>
#include <memory>
#include <thread>

#include <sw/redis++/redis++.h>

#include "handlers/GameRequestHandler.h"
#include "network/HealthCheckServer.h"
#include "persistence/IUserRepository.h"
#include "persistence/Factory/RepositoryFactory.h"
#include "services/GameNodeBridge/GameNodePushPublisher.h"
#include "services/GameNodeBridge/GameNodeRequestRouter.h"
#include "services/GameNodeBridge/LocalReconnectResolver.h"
#include "services/GameSession/GameSessionManager.h"
#include "services/SessionIndex/RedisSessionIndexStore.h"
#include "services/Sharding/RedisGameShardRoutingStore.h"
#include "services/Sharding/RedisShardLoadStore.h"
#include "common/Config/GameNodeConfig.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TimingConfig.h"
#include "common/Logging/Logger.h"

namespace
{
// Runs forever on its own thread -- ticks this shard's own sessions and
// refreshes its own liveness heartbeat (MIGRATION_PLAN.md Phase 4c) every
// GameNodeConfig::SHARD_HEARTBEAT_INTERVAL_MILLIS (coarser than the tick
// interval itself -- a heartbeat doesn't need session-tick precision, and
// refreshing it every single 50ms tick would just be needless Redis
// chatter). Matchmaking's own tick (pairing/timing out queued entries)
// lives in gameallocator/'s runTickLoop now, not here.
void runTickLoop(GameSessionManager& sessionManager, IShardLoadStore& shardLoadStore, const std::string& shardId)
{
    long long millisSinceHeartbeat = 0;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(TimingConfig::SERVER_TICK_INTERVAL_MILLIS));
        sessionManager.tickAll(TimingConfig::SERVER_TICK_INTERVAL_MILLIS);

        millisSinceHeartbeat += TimingConfig::SERVER_TICK_INTERVAL_MILLIS;
        if (millisSinceHeartbeat >= GameNodeConfig::SHARD_HEARTBEAT_INTERVAL_MILLIS)
        {
            shardLoadStore.heartbeat(shardId);
            millisSinceHeartbeat = 0;
        }
    }
}
}

int main()
{
    const char* hostOverride = std::getenv("KUNGFUCHESS_HOST");
    const std::string bindHost = hostOverride ? std::string(hostOverride) : NetworkConfig::DEFAULT_HOST;

    // Same backend-selection pattern as server/src/main.cpp/apigateway/src/
    // main.cpp: SQLite is the actual default, KUNGFUCHESS_POSTGRES_URL opts
    // into Postgres -- must be set the same way as the API Gateway's (see
    // ARCHITECTURE.md's Known gaps), since this process reads users' ratings
    // via the same IUserRepository::findById/setScore the WebSocket Gateway's
    // AuthRequestHandler also reads by id.
    const std::string dbPath = "kungfuchess.db";
    const char* postgresUrl = std::getenv("KUNGFUCHESS_POSTGRES_URL");
    std::unique_ptr<IUserRepository> users = postgresUrl
        ? RepositoryFactory::createUserRepository(RepositoryBackend::Postgres, "", postgresUrl)
        : RepositoryFactory::createUserRepository(RepositoryBackend::Sqlite, dbPath);

    // Mandatory here (unlike server/src/main.cpp's still-opt-in
    // KUNGFUCHESS_REDIS_URL) -- see the file comment above for why a real
    // process split can't tolerate this process falling back to its own
    // local/in-memory session-index storage.
    const char* redisUrl = std::getenv("KUNGFUCHESS_REDIS_URL");
    if (redisUrl == nullptr)
    {
        common::Logger::error(
            "KUNGFUCHESS_REDIS_URL is required for KungFuChessGameNode (MIGRATION_PLAN.md Phase 3) -- "
            "it must point at the same Redis every other KungFuChess process uses.");
        return 1;
    }
    sw::redis::Redis redis(redisUrl);

    RedisSessionIndexStore sessionIndexStore(redis);
    GameNodePushPublisher pushPublisher(redis);

    // This shard's own identity -- see the file comment above.
    // KUNGFUCHESS_SHARD_ID is deliberately *not* mandatory the way
    // KUNGFUCHESS_REDIS_URL is: a single-shard deployment (today's only real
    // one) works fine with the default, since there's nothing to
    // disambiguate yet.
    const char* shardIdEnv = std::getenv("KUNGFUCHESS_SHARD_ID");
    const std::string shardId = shardIdEnv ? std::string(shardIdEnv) : "shard-1";

    RedisShardLoadStore shardLoadStore(redis);
    RedisGameShardRoutingStore shardRoutingStore(redis);
    shardLoadStore.registerShard(shardId);

    GameSessionManager sessionManager(
        [&pushPublisher](const std::string& connectionId, const std::string& json) { pushPublisher.push(connectionId, json); },
        *users,
        sessionIndexStore,
        // MIGRATION_PLAN.md Phase 4c: closes Known gaps #22's cleanup hole --
        // a session that finishes normally (king capture, or an ordinary
        // disconnect-timeout forfeit) now also releases its shard-routing/
        // load bookkeeping, not just its session-index entry, so
        // ShardHealthMonitor never mistakes a long-finished game for one
        // still worth forfeiting when it later scans this shard's sessions.
        [&shardRoutingStore, &shardLoadStore, &shardId](const std::string& sessionId, const std::vector<int>& userIds)
        {
            shardRoutingStore.unbindSession(sessionId, userIds);
            shardLoadStore.decrementLoad(shardId);
        });

    LocalReconnectResolver reconnectResolver(sessionManager);
    GameRequestHandler gameHandler(sessionManager);

    GameNodeRequestRouter requestRouter(
        redis, GameNodeConfig::shardRequestsChannel(shardId), gameHandler, sessionManager, reconnectResolver, pushPublisher);
    requestRouter.start();

    std::thread tickThread(runTickLoop, std::ref(sessionManager), std::ref(shardLoadStore), std::cref(shardId));
    tickThread.detach();

    HealthCheckServer healthCheckServer(NetworkConfig::GAME_NODE_HEALTH_CHECK_PORT, bindHost);
    healthCheckServer.start();
    common::Logger::info(
        "health check listening on http://" + bindHost + ":" + std::to_string(NetworkConfig::GAME_NODE_HEALTH_CHECK_PORT) + "/");

    common::Logger::info("KungFuChess Game Node running as shard '" + shardId + "' (no client-facing port -- see the WebSocket Gateway)");

    // This process has no socket of its own to wait() on -- everything it
    // does happens on the request router's and tick loop's own threads, so
    // just park the main thread forever instead of exiting immediately.
    std::promise<void>().get_future().wait();

    return 0;
}
