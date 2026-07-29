// WebSocket Gateway entry point (MIGRATION_PLAN.md Phase 3 -- this
// executable was KungFuChessServer before this phase, and ran the whole game
// backend in one process; see ARCHITECTURE.md for the full before/after).
// Now a thin relay: it still verifies a login_token (AuthRequestHandler,
// unchanged) and still holds every client's WebSocket connection
// (WebSocketServer, unchanged), but GameSessionManager/Matchmaker/
// GameSession's tick loop have all moved into their own process (gamenode/,
// see gamenode/src/main.cpp) -- this process forwards find_game/move/jump to
// it over Redis pub/sub (GameNodeRequestPublisher/RemoteGameNodeHandler) and
// relays its async replies/pushes back down the right connection
// (GameNodePushRouter -> WebSocketServer::sendTo), instead of calling
// GameSessionManager/Matchmaker in-process the way the monolith used to.
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>

#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

#include "handlers/AuthRequestHandler.h"
#include "network/HealthCheckServer.h"
#include "network/WebSocketServer.h"
#include "persistence/IUserRepository.h"
#include "persistence/Factory/RepositoryFactory.h"
#include "services/Connection/ConnectionRegistry.h"
#include "services/Connection/RedisConnectionStore.h"
#include "services/GameNodeBridge/GameNodePushRouter.h"
#include "services/GameNodeBridge/GameNodeRequestPublisher.h"
#include "services/GameNodeBridge/RemoteGameNodeHandler.h"
#include "services/GameNodeBridge/RemoteReconnectResolver.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TokenConfig.h"
#include "common/Logging/Logger.h"
#include "common/Security/TokenService.h"

namespace
{
// Tries the auth handler first; anything it doesn't recognize (find_game/
// move/jump, or anything else) is simply forwarded to the Game Node --
// classifying "is this matchmaking or a game command" is that process's job
// now (see GameNodeRequestRouter::dispatchClientMessage), not this one's.
std::string dispatch(
    AuthRequestHandler& authHandler,
    RemoteGameNodeHandler& gameNodeHandler,
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

    return gameNodeHandler.handle(connectionId, requestJson);
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
    // today) -- SQLite remains the actual default. AuthRequestHandler only
    // ever reads a user by id here (never writes), same as before this phase.
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

    // Mandatory as of MIGRATION_PLAN.md Phase 3 (previously an opt-in
    // KUNGFUCHESS_REDIS_URL, back when ConnectionRegistry/Matchmaker/
    // GameSessionManager all lived in this same process and a local/
    // in-memory store was a legitimate default). Now that GameSessionManager/
    // Matchmaker live in gamenode/'s own process, this Gateway's
    // ConnectionRegistry MUST be the same Redis-backed store gamenode/ reads
    // -- a local/in-memory ConnectionRegistry here would be invisible to
    // MatchmakingRequestHandler's connectionRegistry.find() call over there,
    // breaking find_game outright. See gamenode/src/main.cpp's file comment
    // for the same requirement on that side.
    const char* redisUrl = std::getenv("KUNGFUCHESS_REDIS_URL");
    if (redisUrl == nullptr)
    {
        common::Logger::error(
            "KUNGFUCHESS_REDIS_URL is required for KungFuChessWsGateway (MIGRATION_PLAN.md Phase 3) -- "
            "it must point at the same Redis the Game Node uses.");
        return 1;
    }
    sw::redis::Redis redis(redisUrl);

    RedisConnectionStore connectionStore(redis);
    ConnectionRegistry connectionRegistry(connectionStore);

    GameNodeRequestPublisher requestPublisher(redis);

    // Constructed before WebSocketServer specifically so its default push
    // handler can capture a real, already-existing server to sendTo through,
    // same deferred-registration reasoning server/main.cpp always used for
    // GameSessionManager's SendToFn before this phase.
    WebSocketServer server(NetworkConfig::DEFAULT_PORT, bindHost);
    HealthCheckServer healthCheckServer(NetworkConfig::HEALTH_CHECK_PORT, bindHost);

    GameNodePushRouter pushRouter(
        redis,
        [&server](const std::string& connectionId, const std::string& json) { server.sendTo(connectionId, json); });
    pushRouter.start();

    RemoteReconnectResolver reconnectResolver(pushRouter, requestPublisher);

    AuthRequestHandler authHandler(
        *users,
        tokenService,
        connectionRegistry,
        reconnectResolver,
        [&server](const std::string& connectionId, const std::string& json) { server.sendTo(connectionId, json); });

    RemoteGameNodeHandler gameNodeHandler(requestPublisher);

    server.setRequestHandler(
        [&authHandler, &gameNodeHandler](const std::string& connectionId, const std::string& requestJson)
        { return dispatch(authHandler, gameNodeHandler, connectionId, requestJson); });

    server.setCloseHandler(
        [&connectionRegistry, &requestPublisher](const std::string& connectionId)
        {
            connectionRegistry.onDisconnected(connectionId);
            requestPublisher.notifyConnectionClosed(connectionId);
        });

    healthCheckServer.start();
    common::Logger::info(
        "health check listening on http://" + bindHost + ":" + std::to_string(NetworkConfig::HEALTH_CHECK_PORT) + "/");

    server.start();
    common::Logger::info(
        "KungFuChess WebSocket Gateway listening on ws://" + bindHost + ":" + std::to_string(NetworkConfig::DEFAULT_PORT));
    server.wait();

    return 0;
}
