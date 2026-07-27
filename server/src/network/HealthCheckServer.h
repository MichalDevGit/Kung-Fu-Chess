#ifndef HEALTH_CHECK_SERVER_H
#define HEALTH_CHECK_SERVER_H

#include <memory>
#include <string>

// A plain HTTP liveness endpoint, deliberately separate from WebSocketServer's
// game-facing WebSocket port: any GET request gets back "200 OK" with a small
// JSON body, with no auth/game-state dependency at all -- an orchestrator
// (Docker HEALTHCHECK, a Kubernetes probe later) can poll this without ever
// speaking the WebSocket/JSON game protocol. Mirrors WebSocketServer's
// pImpl/start-stop shape (see network/WebSocketServer.h) for the same reason:
// nothing outside this file needs to know ixwebsocket's HTTP server types.
class HealthCheckServer
{
public:
    // host semantics match WebSocketServer's: "127.0.0.1" only accepts
    // connections from the same machine/container, "0.0.0.0" accepts from
    // outside (e.g. a published Docker port).
    explicit HealthCheckServer(int port, std::string host = "127.0.0.1");
    ~HealthCheckServer();

    HealthCheckServer(const HealthCheckServer&) = delete;
    HealthCheckServer& operator=(const HealthCheckServer&) = delete;

    // Binds the port and starts accepting requests in the background.
    // Throws std::runtime_error if the port can't be bound.
    void start();

    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
