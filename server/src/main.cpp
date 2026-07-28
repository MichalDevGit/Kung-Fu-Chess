// KungFuChess server entry point: opens the SQLite-backed user database,
// wires it through AuthRequestHandler (login_token verification only --
// register/password login now live in the separate apigateway/ process, see
// MIGRATION_PLAN.md Phase 2), ConnectionRegistry/Matchmaker/
// MatchmakingRequestHandler for pairing authenticated connections into
// matches, and GameSessionManager/GameRequestHandler for game commands, all
// dispatched from one WebSocket handler; also runs the server-owned tick
// loop that advances every active GameSession's real-time clock and scans
// the matchmaking queue, independent of whether any client happens to be
// connected right now.
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>
#include <thread>

#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

#include "handlers/AuthRequestHandler.h"
#include "handlers/GameRequestHandler.h"
#include "handlers/MatchmakingRequestHandler.h"
#include "network/HealthCheckServer.h"
#include "network/WebSocketServer.h"
#include "persistence/IUserRepository.h"
#include "persistence/RepositoryFactory.h"
#include "services/ConnectionRegistry.h"
#include "services/GameSessionManager.h"
#include "services/IConnectionStore.h"
#include "services/IMatchQueueStore.h"
#include "services/ISessionIndexStore.h"
#include "services/LocalConnectionStore.h"
#include "services/LocalMatchQueueStore.h"
#include "services/LocalSessionIndexStore.h"
#include "services/Matchmaker.h"
#include "services/RedisConnectionStore.h"
#include "services/RedisMatchQueueStore.h"
#include "services/RedisSessionIndexStore.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TimingConfig.h"
#include "common/Config/TokenConfig.h"
#include "common/Logging/Logger.h"
#include "common/MonotonicClock.h"
#include "common/Security/TokenService.h"
#include "common/enums/PieceColor.h"

namespace
{
// Tries the auth handler first, then matchmaking, and only falls back to the
// game handler when neither recognizes the message type. Each handler stays
// ignorant of the others' message-type sets -- adding a new type to any one
// of them never requires touching this dispatch.
std::string dispatch(
    AuthRequestHandler& authHandler,
    MatchmakingRequestHandler& matchmakingHandler,
    GameRequestHandler& gameHandler,
    const std::string& connectionId,
    const std::string& requestJson)
{
    auto isUnknownType = [](const std::string& responseJson)
    {
        const nlohmann::json parsed = nlohmann::json::parse(responseJson);
        return parsed.value("type", std::string()) == protocol::MessageType::Error &&
               parsed.value("error", std::string()) == "unknown_type";
    };

    const std::string authResponse = authHandler.handle(connectionId, requestJson);
    if (!isUnknownType(authResponse))
        return authResponse;

    const std::string matchmakingResponse = matchmakingHandler.handle(connectionId, requestJson);
    if (!isUnknownType(matchmakingResponse))
        return matchmakingResponse;

    return gameHandler.handle(connectionId, requestJson);
}

// Runs forever on its own thread: advances every active GameSession's clock
// on a fixed interval (replaces the old GameLoop-driven wall-clock
// controller.wait(deltaMs) call, since the authoritative clock must advance
// even when no client is connected/rendering) and scans the matchmaking
// queue on the same cadence.
void runTickLoop(GameSessionManager& sessionManager, Matchmaker& matchmaker, WebSocketServer& server)
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

                server.sendTo(
                    match.first.connectionId,
                    protocol::MatchFoundResult{session.getId(), PieceColor::White, match.second.username, session.getGameView()}.toJson());

                server.sendTo(
                    match.second.connectionId,
                    protocol::MatchFoundResult{session.getId(), PieceColor::Black, match.first.username, session.getGameView()}.toJson());
            },
            [&](const Matchmaker::Entry& entry)
            {
                server.sendTo(entry.connectionId, protocol::NoMatchResult{}.toJson());
            });
    }
}
}

int main()
{
    // "127.0.0.1" (matching NetworkConfig::DEFAULT_HOST) only accepts
    // connections from the same machine/container; KUNGFUCHESS_HOST lets a
    // container runtime (see Dockerfile/docker-compose.yml) override this to
    // "0.0.0.0" so a published port is actually reachable from outside the
    // container. Unset locally, so native/dev behavior is unchanged.
    const char* hostOverride = std::getenv("KUNGFUCHESS_HOST");
    const std::string bindHost = hostOverride ? std::string(hostOverride) : NetworkConfig::DEFAULT_HOST;

    // KUNGFUCHESS_POSTGRES_URL is an explicit opt-in only (unset everywhere
    // today) -- SQLite remains the actual default per MIGRATION_PLAN.md
    // Phase 1 ("keep SQLite as the default backend until Postgres is
    // verified in a staging run"). Setting it lets a staging deployment
    // exercise PostgresUserRepository with zero other code changes.
    const std::string dbPath = "kungfuchess.db";
    const char* postgresUrl = std::getenv("KUNGFUCHESS_POSTGRES_URL");
    std::unique_ptr<IUserRepository> users = postgresUrl
        ? RepositoryFactory::createUserRepository(RepositoryBackend::Postgres, "", postgresUrl)
        : RepositoryFactory::createUserRepository(RepositoryBackend::Sqlite, dbPath);

    // Must match the API Gateway process's own KUNGFUCHESS_TOKEN_SECRET
    // (or dev-default fallback) -- see ARCHITECTURE.md's Known gaps and
    // MIGRATION_PLAN.md Phase 2. This process only ever verifies a token
    // (login_token); it never issues one.
    const char* tokenSecretEnv = std::getenv("KUNGFUCHESS_TOKEN_SECRET");
    const std::string tokenSecret =
        tokenSecretEnv ? std::string(tokenSecretEnv) : TokenConfig::DEV_INSECURE_DEFAULT_SECRET;
    security::TokenService tokenService(tokenSecret);

    // KUNGFUCHESS_REDIS_URL is an explicit opt-in only (unset everywhere
    // today) -- local/in-memory storage remains the actual default per
    // MIGRATION_PLAN.md Phase 1 ("still passes today's tests unchanged").
    // Setting it opts ConnectionRegistry/Matchmaker/GameSessionManager's
    // lookup indexes into Redis all at once, sharing one connection;
    // GameSessionManager's own `sessions` map of live GameSession objects
    // stays in-process only either way.
    const char* redisUrl = std::getenv("KUNGFUCHESS_REDIS_URL");
    std::optional<sw::redis::Redis> redis;
    if (redisUrl)
        redis.emplace(redisUrl);

    std::unique_ptr<IConnectionStore> connectionStore = redisUrl
        ? std::unique_ptr<IConnectionStore>(std::make_unique<RedisConnectionStore>(*redis))
        : std::unique_ptr<IConnectionStore>(std::make_unique<LocalConnectionStore>());
    std::unique_ptr<IMatchQueueStore> matchQueueStore = redisUrl
        ? std::unique_ptr<IMatchQueueStore>(std::make_unique<RedisMatchQueueStore>(*redis))
        : std::unique_ptr<IMatchQueueStore>(std::make_unique<LocalMatchQueueStore>());
    std::unique_ptr<ISessionIndexStore> sessionIndexStore = redisUrl
        ? std::unique_ptr<ISessionIndexStore>(std::make_unique<RedisSessionIndexStore>(*redis))
        : std::unique_ptr<ISessionIndexStore>(std::make_unique<LocalSessionIndexStore>());

    ConnectionRegistry connectionRegistry(*connectionStore);
    Matchmaker matchmaker(*matchQueueStore);

    // Constructed before GameSessionManager/AuthRequestHandler (and without
    // a request handler yet -- see setRequestHandler below) specifically so
    // the sendTo lambdas passed to them can capture a real, already-existing
    // server to push through, instead of the console-log stub this used to be.
    WebSocketServer server(NetworkConfig::DEFAULT_PORT, bindHost);
    HealthCheckServer healthCheckServer(NetworkConfig::HEALTH_CHECK_PORT, bindHost);

    GameSessionManager sessionManager(
        [&server](const std::string& connectionId, const std::string& json)
        {
            server.sendTo(connectionId, json);
        },
        *users,
        *sessionIndexStore);

    // Needs sessionManager (to detect/resume a reconnecting player's
    // existing session on login -- see the class comment) and a way to push
    // that resume independent of login's own synchronous reply.
    AuthRequestHandler authHandler(
        *users,
        tokenService,
        connectionRegistry,
        sessionManager,
        [&server](const std::string& connectionId, const std::string& json)
        {
            server.sendTo(connectionId, json);
        });

    MatchmakingRequestHandler matchmakingHandler(connectionRegistry, matchmaker, sessionManager);
    GameRequestHandler gameHandler(sessionManager);

    std::thread tickThread(runTickLoop, std::ref(sessionManager), std::ref(matchmaker), std::ref(server));
    tickThread.detach();

    server.setRequestHandler(
        [&authHandler, &matchmakingHandler, &gameHandler](const std::string& connectionId, const std::string& requestJson)
        { return dispatch(authHandler, matchmakingHandler, gameHandler, connectionId, requestJson); });

    server.setCloseHandler(
        [&connectionRegistry, &matchmaker, &sessionManager](const std::string& connectionId)
        {
            connectionRegistry.onDisconnected(connectionId);
            matchmaker.removeByConnection(connectionId);
            sessionManager.onConnectionClosed(connectionId);
        });

    healthCheckServer.start();
    common::Logger::info(
        "health check listening on http://" + bindHost + ":" + std::to_string(NetworkConfig::HEALTH_CHECK_PORT) + "/");

    server.start();
    common::Logger::info(
        "KungFuChess server listening on ws://" + bindHost + ":" + std::to_string(NetworkConfig::DEFAULT_PORT));
    server.wait();

    return 0;
}
