// Game Node entry point (MIGRATION_PLAN.md Phase 3): owns GameSessionManager,
// Matchmaker, and GameSession's tick loop -- exactly the same classes
// server/src/main.cpp used to construct and drive directly, just no longer
// wired to a WebSocketServer it doesn't hold. Every push a GameSession/
// Matchmaker used to send via server.sendTo now goes out over
// GameNodeConfig::PUSHES_CHANNEL instead (GameNodePushPublisher); every
// find_game/move/jump/connection-close the WebSocket Gateway's own process
// used to dispatch in-process now arrives over GameNodeConfig::
// REQUESTS_CHANNEL instead (GameNodeRequestRouter). See ARCHITECTURE.md for
// the full design.
//
// Unlike server/src/main.cpp (where KUNGFUCHESS_REDIS_URL is still an
// opt-in), Redis is mandatory here: this process's ConnectionRegistry/
// Matchmaker/GameSessionManager index stores MUST be the same Redis the
// WebSocket Gateway's are pointed at, or the two processes silently diverge
// on "who's connected"/"who's queued" -- the same class of problem
// MIGRATION_PLAN.md Phase 2 flagged for KUNGFUCHESS_POSTGRES_URL, just for
// Redis now that an actual process split exists.
#include <chrono>
#include <cstdlib>
#include <future>
#include <memory>
#include <optional>
#include <thread>

#include <sw/redis++/redis++.h>

#include "handlers/GameRequestHandler.h"
#include "handlers/MatchmakingRequestHandler.h"
#include "network/HealthCheckServer.h"
#include "persistence/IUserRepository.h"
#include "persistence/Factory/RepositoryFactory.h"
#include "services/Connection/ConnectionRegistry.h"
#include "services/Connection/RedisConnectionStore.h"
#include "services/GameNodeBridge/GameNodePushPublisher.h"
#include "services/GameNodeBridge/GameNodeRequestRouter.h"
#include "services/GameNodeBridge/LocalReconnectResolver.h"
#include "services/GameSession/GameSessionManager.h"
#include "services/Matchmaking/Matchmaker.h"
#include "services/Matchmaking/RedisMatchQueueStore.h"
#include "services/SessionIndex/RedisSessionIndexStore.h"
#include "protocol/Message.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TimingConfig.h"
#include "common/Logging/Logger.h"
#include "common/MonotonicClock.h"
#include "common/enums/PieceColor.h"

namespace
{
// Runs forever on its own thread -- the exact same loop server/src/main.cpp
// used to run, just publishing match outcomes via GameNodePushPublisher
// instead of calling a WebSocketServer directly.
void runTickLoop(GameSessionManager& sessionManager, Matchmaker& matchmaker, GameNodePushPublisher& pushPublisher)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(TimingConfig::SERVER_TICK_INTERVAL_MILLIS));
        sessionManager.tickAll(TimingConfig::SERVER_TICK_INTERVAL_MILLIS);

        matchmaker.tick(
            nowMillis(),
            [&](const Matchmaker::Match& match)
            {
                // match.first is always the earlier-enqueued of the pair
                // (see Matchmaker::tick) -- White = entered first.
                GameSession::Player white{match.first.userId, match.first.username, match.first.connectionId};
                GameSession::Player black{match.second.userId, match.second.username, match.second.connectionId};

                GameSession& session = sessionManager.createSession(white, black);

                pushPublisher.push(
                    match.first.connectionId,
                    protocol::MatchFoundResult{session.getId(), PieceColor::White, match.second.username, session.getGameView()}
                        .toJson());

                pushPublisher.push(
                    match.second.connectionId,
                    protocol::MatchFoundResult{session.getId(), PieceColor::Black, match.first.username, session.getGameView()}
                        .toJson());
            },
            [&](const Matchmaker::Entry& entry)
            {
                pushPublisher.push(entry.connectionId, protocol::NoMatchResult{}.toJson());
            });
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
    // local/in-memory ConnectionRegistry/Matchmaker/session-index storage.
    const char* redisUrl = std::getenv("KUNGFUCHESS_REDIS_URL");
    if (redisUrl == nullptr)
    {
        common::Logger::error(
            "KUNGFUCHESS_REDIS_URL is required for KungFuChessGameNode (MIGRATION_PLAN.md Phase 3) -- "
            "it must point at the same Redis the WebSocket Gateway uses.");
        return 1;
    }
    sw::redis::Redis redis(redisUrl);

    RedisConnectionStore connectionStore(redis);
    RedisMatchQueueStore matchQueueStore(redis);
    RedisSessionIndexStore sessionIndexStore(redis);

    ConnectionRegistry connectionRegistry(connectionStore);
    Matchmaker matchmaker(matchQueueStore);

    GameNodePushPublisher pushPublisher(redis);

    GameSessionManager sessionManager(
        [&pushPublisher](const std::string& connectionId, const std::string& json) { pushPublisher.push(connectionId, json); },
        *users,
        sessionIndexStore);

    LocalReconnectResolver reconnectResolver(sessionManager);
    MatchmakingRequestHandler matchmakingHandler(connectionRegistry, matchmaker, sessionManager);
    GameRequestHandler gameHandler(sessionManager);

    GameNodeRequestRouter requestRouter(
        redis, matchmakingHandler, gameHandler, matchmaker, sessionManager, reconnectResolver, pushPublisher);
    requestRouter.start();

    std::thread tickThread(runTickLoop, std::ref(sessionManager), std::ref(matchmaker), std::ref(pushPublisher));
    tickThread.detach();

    HealthCheckServer healthCheckServer(NetworkConfig::GAME_NODE_HEALTH_CHECK_PORT, bindHost);
    healthCheckServer.start();
    common::Logger::info(
        "health check listening on http://" + bindHost + ":" + std::to_string(NetworkConfig::GAME_NODE_HEALTH_CHECK_PORT) + "/");

    common::Logger::info("KungFuChess Game Node running (no client-facing port -- see the WebSocket Gateway)");

    // This process has no socket of its own to wait() on -- everything it
    // does happens on the request router's and tick loop's own threads, so
    // just park the main thread forever instead of exiting immediately.
    std::promise<void>().get_future().wait();

    return 0;
}
