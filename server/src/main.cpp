// KungFuChess server entry point: opens the SQLite-backed user database,
// wires it through AuthService/AuthRequestHandler for auth requests, and
// GameSessionManager/GameRequestHandler for game commands, both dispatched
// from one WebSocket handler; also runs the server-owned tick loop that
// advances every active GameSession's real-time clock independent of
// whether any client happens to be connected right now.
#include <chrono>
#include <iostream>
#include <thread>

#include <nlohmann/json.hpp>

#include "handlers/AuthRequestHandler.h"
#include "handlers/GameRequestHandler.h"
#include "network/WebSocketServer.h"
#include "persistence/Database.h"
#include "persistence/UserRepository.h"
#include "services/AuthService.h"
#include "services/GameSessionManager.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TimingConfig.h"

namespace
{
// Tries the auth handler first; only falls back to the game handler when
// the auth handler doesn't recognize the message type at all. This way
// neither handler needs to know the other's message-type set -- adding a
// new message type to either one never requires touching this dispatch.
std::string dispatch(AuthRequestHandler& authHandler, GameRequestHandler& gameHandler, const std::string& requestJson)
{
    const std::string authResponse = authHandler.handle(requestJson);
    const nlohmann::json parsedAuthResponse = nlohmann::json::parse(authResponse);

    const bool isUnknownToAuth =
        parsedAuthResponse.value("type", std::string()) == protocol::MessageType::Error &&
        parsedAuthResponse.value("error", std::string()) == "unknown_type";

    return isUnknownToAuth ? gameHandler.handle(requestJson) : authResponse;
}

// Runs forever on its own thread, advancing every active GameSession's clock
// on a fixed interval -- replaces the old GameLoop-driven wall-clock
// controller.wait(deltaMs) call, since the authoritative clock must now
// advance even when no client is connected/rendering.
void runTickLoop(GameSessionManager& sessionManager)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(TimingConfig::SERVER_TICK_INTERVAL_MILLIS));
        sessionManager.tickAll(TimingConfig::SERVER_TICK_INTERVAL_MILLIS);
    }
}
}

int main()
{
    const std::string dbPath = "kungfuchess.db";
    Database database(dbPath);
    UserRepository users(database);
    AuthService authService(users);
    AuthRequestHandler authHandler(authService);

    // Constructed before GameSessionManager (and without a request handler
    // yet -- see setRequestHandler below) specifically so the broadcast
    // lambda passed to GameSessionManager can capture a real, already-
    // existing server to push through, instead of the console-log stub this
    // used to be.
    WebSocketServer server(NetworkConfig::DEFAULT_PORT);

    GameSessionManager sessionManager([&server](const std::string& json)
        {
            server.broadcast(json);
        });
    GameRequestHandler gameHandler(sessionManager);

    std::thread tickThread(runTickLoop, std::ref(sessionManager));
    tickThread.detach();

    server.setRequestHandler([&authHandler, &gameHandler](const std::string& requestJson)
                              { return dispatch(authHandler, gameHandler, requestJson); });

    std::cout << "KungFuChess server listening on ws://" << NetworkConfig::DEFAULT_HOST
               << ":" << NetworkConfig::DEFAULT_PORT << "\n";
    server.start();
    server.wait();

    return 0;
}
