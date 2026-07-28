#ifndef API_GATEWAY_SERVER_H
#define API_GATEWAY_SERVER_H

#include <memory>
#include <string>

class ApiGatewayRequestHandler;

// A plain HTTP REST endpoint (POST /register, POST /login), deliberately
// separate from the game's WebSocket port (server/src/network/WebSocketServer)
// and its health-check port (server/src/network/HealthCheckServer) -- see
// MIGRATION_PLAN.md Phase 2. Mirrors HealthCheckServer's pImpl/start-stop
// shape for the same reason: nothing outside this file needs to know
// ixwebsocket's HTTP server types. Routing itself is deliberately trivial
// (two paths, dispatched by method+uri) -- the actual request-parsing/
// business logic lives in ApiGatewayRequestHandler, which is unit-testable
// without this class at all.
class ApiGatewayServer
{
public:
    // host semantics match WebSocketServer's/HealthCheckServer's:
    // "127.0.0.1" only accepts connections from the same machine/container,
    // "0.0.0.0" accepts from outside (e.g. a published Docker port).
    ApiGatewayServer(ApiGatewayRequestHandler& handler, int port, std::string host = "127.0.0.1");
    ~ApiGatewayServer();

    ApiGatewayServer(const ApiGatewayServer&) = delete;
    ApiGatewayServer& operator=(const ApiGatewayServer&) = delete;

    // Binds the port and starts accepting requests in the background.
    // Throws std::runtime_error if the port can't be bound.
    void start();

    // Blocks the calling thread until stop() is called.
    void wait();

    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
