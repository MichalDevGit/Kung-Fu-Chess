// WebSocket Gateway entry point (MIGRATION_PLAN.md Phase 3 -- this
// executable was KungFuChessServer before this phase, and ran the whole game
// backend in one process; see ARCHITECTURE.md for the full before/after).
// Now a thin relay: it still verifies a login_token (AuthRequestHandler,
// unchanged) and still holds every client's WebSocket connection
// (WebSocketServer, unchanged), but GameSessionManager/Matchmaker/
// GameSession's tick loop have all moved into their own processes (a
// singleton gameallocator/ for Matchmaker, one or more gamenode/ Game Server
// Shards for GameSessionManager -- MIGRATION_PLAN.md Phase 4b) -- this
// process forwards find_game/move/jump over Redis pub/sub
// (GameNodeRequestPublisher/GatewayGameRouter) and relays async replies/
// pushes back down the right connection (GameNodePushRouter ->
// WebSocketServer::sendTo), instead of calling GameSessionManager/Matchmaker
// in-process the way the monolith used to. Phase 4b: GatewayGameRouter now
// decides *which* process a given request goes to (the one Allocator for
// find_game, or a specific shard for move/jump, resolved via
// RedisSessionIndexStore + RedisGameShardRoutingStore) -- Phase 3's version
// of this file forwarded everything to a single Game Node unconditionally,
// which only worked because exactly one ever ran.
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
#include "services/GameNodeBridge/GatewayGameRouter.h"
#include "services/GameNodeBridge/RemoteReconnectResolver.h"
#include "services/SessionIndex/RedisSessionIndexStore.h"
#include "services/Sharding/RedisGameShardRoutingStore.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TokenConfig.h"
#include "common/Logging/Logger.h"
#include "common/Security/TokenService.h"

namespace
{
// Tries the auth handler first; anything it doesn't recognize (find_game/
// move/jump, or anything else) is routed by GatewayGameRouter -- Phase 4b:
// this Gateway now decides for itself whether a request goes to the one
// Game Allocator or to a specific Game Server Shard (see
// GatewayGameRouter::handle), rather than forwarding everything to a single
// Game Node and letting that process classify it.
std::string dispatch(
    AuthRequestHandler& authHandler,
    GatewayGameRouter& gameRouter,
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

    return gameRouter.handle(connectionId, requestJson);
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
    // Matchmaker live in gameallocator/'s and gamenode/'s own processes
    // (Phase 4b), this Gateway's ConnectionRegistry/RedisSessionIndexStore/
    // RedisGameShardRoutingStore MUST be the same Redis-backed stores those
    // processes read/write -- a local/in-memory ConnectionRegistry here
    // would be invisible to MatchmakingRequestHandler's
    // connectionRegistry.find() call over there, breaking find_game
    // outright. See gameallocator/src/main.cpp's and gamenode/src/main.cpp's
    // file comments for the same requirement on those sides.
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

    // Phase 4b: read-only lookups this Gateway needs to route a move/jump/
    // connection_closed/reconnect_check to the right process instead of
    // assuming "the one Game Node" -- both are written elsewhere
    // (RedisSessionIndexStore by GameSessionManager on whichever shard,
    // RedisGameShardRoutingStore by GameAllocator), this process only ever
    // reads them.
    RedisSessionIndexStore sessionIndexStore(redis);
    RedisGameShardRoutingStore shardRoutingStore(redis);

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

    RemoteReconnectResolver reconnectResolver(pushRouter, requestPublisher, shardRoutingStore);

    AuthRequestHandler authHandler(
        *users,
        tokenService,
        connectionRegistry,
        reconnectResolver,
        [&server](const std::string& connectionId, const std::string& json) { server.sendTo(connectionId, json); });

    GatewayGameRouter gameRouter(requestPublisher, sessionIndexStore, shardRoutingStore);

    server.setRequestHandler(
        [&authHandler, &gameRouter](const std::string& connectionId, const std::string& requestJson)
        { return dispatch(authHandler, gameRouter, connectionId, requestJson); });

    server.setCloseHandler(
        [&connectionRegistry, &gameRouter](const std::string& connectionId)
        {
            connectionRegistry.onDisconnected(connectionId);
            gameRouter.notifyConnectionClosed(connectionId);
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
