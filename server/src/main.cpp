// KungFuChess server entry point: opens the SQLite-backed user database,
// wires it through AuthService/AuthRequestHandler, and serves auth requests
// over a plain ws:// WebSocket on port 9002 until the process is killed.
#include <iostream>

#include "handlers/AuthRequestHandler.h"
#include "network/WebSocketServer.h"
#include "persistence/Database.h"
#include "persistence/UserRepository.h"
#include "services/AuthService.h"

namespace
{
    constexpr int kPort = 9002;
}

int main()
{
    const std::string dbPath = "kungfuchess.db";
    Database database(dbPath);
    UserRepository users(database);
    AuthService authService(users);
    AuthRequestHandler handler(authService);

    WebSocketServer server(kPort, [&handler](const std::string& requestJson)
                            { return handler.handle(requestJson); });

    std::cout << "KungFuChess server listening on ws://127.0.0.1:" << kPort << "\n";
    server.start();
    server.wait();

    return 0;
}
