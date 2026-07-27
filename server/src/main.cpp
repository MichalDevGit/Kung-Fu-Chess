// KungFuChess server entry point: opens the SQLite-backed user database,
// wires it through AuthService/AuthRequestHandler for auth requests,
// ConnectionRegistry/Matchmaker/MatchmakingRequestHandler for pairing
// authenticated connections into matches, and GameSessionManager/
// GameRequestHandler for game commands, all dispatched from one WebSocket
// handler; also runs the server-owned tick loop that advances every active
// GameSession's real-time clock and scans the matchmaking queue,
// independent of whether any client happens to be connected right now.
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <nlohmann/json.hpp>

#include "handlers/AuthRequestHandler.h"
#include "handlers/GameRequestHandler.h"
#include "handlers/MatchmakingRequestHandler.h"
#include "network/WebSocketServer.h"
#include "persistence/IUserRepository.h"
#include "persistence/RepositoryFactory.h"
#include "services/AuthService.h"
#include "services/ConnectionRegistry.h"
#include "services/GameSessionManager.h"
#include "services/Matchmaker.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TimingConfig.h"
#include "common/MonotonicClock.h"
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
    const std::string dbPath = "kungfuchess.db";
    std::unique_ptr<IUserRepository> users =
        RepositoryFactory::createUserRepository(RepositoryBackend::Sqlite, dbPath);
    AuthService authService(*users);
    ConnectionRegistry connectionRegistry;
    Matchmaker matchmaker;

    // Constructed before GameSessionManager/AuthRequestHandler (and without
    // a request handler yet -- see setRequestHandler below) specifically so
    // the sendTo lambdas passed to them can capture a real, already-existing
    // server to push through, instead of the console-log stub this used to be.
    WebSocketServer server(NetworkConfig::DEFAULT_PORT);

    GameSessionManager sessionManager(
        [&server](const std::string& connectionId, const std::string& json)
        {
            server.sendTo(connectionId, json);
        },
        *users);

    // Needs sessionManager (to detect/resume a reconnecting player's
    // existing session on login -- see the class comment) and a way to push
    // that resume independent of login's own synchronous reply.
    AuthRequestHandler authHandler(
        authService,
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

    std::cout << "KungFuChess server listening on ws://" << NetworkConfig::DEFAULT_HOST
               << ":" << NetworkConfig::DEFAULT_PORT << "\n";
    server.start();
    server.wait();

    return 0;
}
