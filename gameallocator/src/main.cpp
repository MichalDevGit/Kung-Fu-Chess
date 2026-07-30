// Game Allocator entry point (MIGRATION_PLAN.md Phase 4b): the one process
// that owns Matchmaker/MatchmakingRequestHandler/GameAllocator -- split out
// of gamenode/, which used to own these alongside GameSessionManager back
// when Phase 3 only ever ran one Game Node, so there was no distinction yet
// between "the matchmaking singleton" and "a shard." This is the one true
// singleton in the whole topology: exactly one of these ever runs, no
// matter how many Game Server Shard (gamenode/) replicas exist, since a
// second Matchmaker instance racing the first over the same Redis-backed
// queue is exactly the kind of split-brain MIGRATION_PLAN.md Phase 1
// externalized the queue store to make survivable, not something to
// actually run twice on purpose.
//
// Every push Matchmaker::tick's outcome used to send directly (in the
// monolith) or via gamenode/'s GameNodePushPublisher (Phase 3) now goes out
// the same way from here (this process's own GameNodePushPublisher); every
// find_game/connection-close the WebSocket Gateway used to forward to a
// single Game Node now arrives over GameNodeConfig::MATCHMAKING_REQUESTS_CHANNEL
// instead (GameAllocatorRequestRouter). See ARCHITECTURE.md for the full
// design, and GameAllocator's own class comment for why session creation on
// whichever shard gets picked is fire-and-forget rather than a round trip.
//
// Redis is mandatory here, same reasoning as gamenode/src/main.cpp: this
// process's ConnectionRegistry/Matchmaker queue/session-index/shard-load/
// shard-routing stores MUST be the same Redis every other KungFuChess
// process is pointed at, or they silently diverge on "who's connected"/
// "who's queued"/"which shard hosts this session."
#include <chrono>
#include <cstdlib>
#include <future>
#include <thread>

#include <sw/redis++/redis++.h>

#include "handlers/MatchmakingRequestHandler.h"
#include "network/HealthCheckServer.h"
#include "services/Connection/ConnectionRegistry.h"
#include "services/Connection/RedisConnectionStore.h"
#include "services/GameNodeBridge/GameAllocatorRequestRouter.h"
#include "services/GameNodeBridge/GameNodePushPublisher.h"
#include "services/GameNodeBridge/GameNodeRequestPublisher.h"
#include "services/GameSession/GameSession.h"
#include "services/Matchmaking/Matchmaker.h"
#include "services/Matchmaking/RedisMatchQueueStore.h"
#include "services/SessionIndex/RedisSessionIndexStore.h"
#include "services/Sharding/GameAllocator.h"
#include "services/Sharding/RedisGameShardRoutingStore.h"
#include "services/Sharding/RedisShardLoadStore.h"
#include "services/Sharding/ShardHealthMonitor.h"
#include "protocol/Message.h"
#include "common/Config/GameNodeConfig.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TimingConfig.h"
#include "common/Logging/Logger.h"
#include "common/MonotonicClock.h"
#include "common/enums/PieceColor.h"

namespace
{
// Runs forever on its own thread -- the exact same matchmaking tick
// gamenode/'s runTickLoop used to also run before Phase 4b split it out.
// onMatched now goes through GameAllocator, which picks a shard, fires off
// a fire-and-forget session-creation request to it, and hands back the new
// session's id plus its (always-deterministic) initial GameView -- see
// GameAllocator's class comment for why no reply from the shard is needed
// before pushing MatchFoundResult to both clients. MIGRATION_PLAN.md
// Phase 4c: also runs ShardHealthMonitor's dead-shard check on the same
// coarser cadence gamenode/'s heartbeat refresh uses
// (GameNodeConfig::SHARD_HEARTBEAT_INTERVAL_MILLIS) -- no need to poll every
// single tick, since a shard can only actually go stale on the scale of its
// heartbeat TTL, several seconds out.
void runTickLoop(Matchmaker& matchmaker, GameAllocator& allocator, GameNodePushPublisher& pushPublisher, ShardHealthMonitor& healthMonitor)
{
    long long millisSinceHealthCheck = 0;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(TimingConfig::SERVER_TICK_INTERVAL_MILLIS));

        matchmaker.tick(
            nowMillis(),
            [&](const Matchmaker::Match& match)
            {
                // match.first is always the earlier-enqueued of the pair
                // (see Matchmaker::tick) -- White = entered first.
                const GameAllocator::Allocation allocation = allocator.allocate(match);

                pushPublisher.push(
                    match.first.connectionId,
                    protocol::MatchFoundResult{allocation.sessionId, PieceColor::White, match.second.username, allocation.initialView}
                        .toJson());

                pushPublisher.push(
                    match.second.connectionId,
                    protocol::MatchFoundResult{allocation.sessionId, PieceColor::Black, match.first.username, allocation.initialView}
                        .toJson());
            },
            [&](const Matchmaker::Entry& entry)
            {
                pushPublisher.push(entry.connectionId, protocol::NoMatchResult{}.toJson());
            });

        millisSinceHealthCheck += TimingConfig::SERVER_TICK_INTERVAL_MILLIS;
        if (millisSinceHealthCheck >= GameNodeConfig::SHARD_HEARTBEAT_INTERVAL_MILLIS)
        {
            healthMonitor.checkAndForfeitDeadShards();
            millisSinceHealthCheck = 0;
        }
    }
}
}

int main()
{
    const char* hostOverride = std::getenv("KUNGFUCHESS_HOST");
    const std::string bindHost = hostOverride ? std::string(hostOverride) : NetworkConfig::DEFAULT_HOST;

    const char* redisUrl = std::getenv("KUNGFUCHESS_REDIS_URL");
    if (redisUrl == nullptr)
    {
        common::Logger::error(
            "KUNGFUCHESS_REDIS_URL is required for KungFuChessGameAllocator (MIGRATION_PLAN.md Phase 4b) -- "
            "it must point at the same Redis every other KungFuChess process uses.");
        return 1;
    }
    sw::redis::Redis redis(redisUrl);

    RedisConnectionStore connectionStore(redis);
    RedisMatchQueueStore matchQueueStore(redis);
    RedisSessionIndexStore sessionIndexStore(redis);
    RedisShardLoadStore shardLoadStore(redis);
    RedisGameShardRoutingStore shardRoutingStore(redis);

    ConnectionRegistry connectionRegistry(connectionStore);
    Matchmaker matchmaker(matchQueueStore);

    GameNodePushPublisher pushPublisher(redis);
    GameNodeRequestPublisher requestPublisher(redis);

    GameAllocator allocator(
        shardLoadStore,
        shardRoutingStore,
        [&requestPublisher](const std::string& sessionId, GameSession::Player white, GameSession::Player black, const std::string& shardId)
        {
            requestPublisher.requestSessionCreation(
                GameNodeConfig::shardRequestsChannel(shardId),
                sessionId,
                white.userId,
                white.username,
                white.connectionId,
                black.userId,
                black.username,
                black.connectionId);
        });

    // ISessionIndexStore& here only for MatchmakingRequestHandler's
    // "already_in_game" guard (a read-only lookup against the same
    // Redis-backed store every Game Server Shard's GameSessionManager writes
    // to) -- this process never binds/unbinds a session itself.
    MatchmakingRequestHandler matchmakingHandler(connectionRegistry, matchmaker, sessionIndexStore);

    GameAllocatorRequestRouter requestRouter(redis, matchmakingHandler, matchmaker, pushPublisher);
    requestRouter.start();

    // MIGRATION_PLAN.md Phase 4c: forfeits every session a shard was hosting
    // once its heartbeat lapses -- see ShardHealthMonitor's class comment for
    // why this needs no IUserRepository/RatingService of its own.
    ShardHealthMonitor shardHealthMonitor(
        shardLoadStore,
        shardRoutingStore,
        [&connectionRegistry](int userId) { return connectionRegistry.findConnectionForUser(userId); },
        [&pushPublisher](const std::string& connectionId, const std::string& json) { pushPublisher.push(connectionId, json); });

    std::thread tickThread(
        runTickLoop, std::ref(matchmaker), std::ref(allocator), std::ref(pushPublisher), std::ref(shardHealthMonitor));
    tickThread.detach();

    HealthCheckServer healthCheckServer(NetworkConfig::GAME_ALLOCATOR_HEALTH_CHECK_PORT, bindHost);
    healthCheckServer.start();
    common::Logger::info(
        "health check listening on http://" + bindHost + ":" + std::to_string(NetworkConfig::GAME_ALLOCATOR_HEALTH_CHECK_PORT) + "/");

    common::Logger::info("KungFuChess Game Allocator running (no client-facing port -- see the WebSocket Gateway)");

    // This process has no socket of its own to wait() on -- everything it
    // does happens on the request router's and tick loop's own threads, so
    // just park the main thread forever instead of exiting immediately.
    std::promise<void>().get_future().wait();

    return 0;
}
