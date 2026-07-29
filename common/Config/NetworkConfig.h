#ifndef COMMON_CONFIG_NETWORK_CONFIG_H
#define COMMON_CONFIG_NETWORK_CONFIG_H

// Single source of truth for where the server listens and where the client
// connects. Previously the host/port were separately hardcoded in
// server/src/main.cpp and client/src/main.cpp.
namespace NetworkConfig
{
    constexpr const char* DEFAULT_HOST = "127.0.0.1";
    constexpr int DEFAULT_PORT = 9002;

    // Plain HTTP liveness endpoint (server/src/network/HealthCheckServer),
    // separate from the game's WebSocket port -- lets an orchestrator (Docker
    // healthcheck, Kubernetes probe) check "is this process up" without
    // speaking the WebSocket/JSON protocol at all.
    constexpr int HEALTH_CHECK_PORT = 9003;

    // Plain HTTP REST API (apigateway/src/network/ApiGatewayServer) --
    // register/login now live here instead of on the WebSocket process (see
    // MIGRATION_PLAN.md Phase 2). A separate port/host from DEFAULT_PORT
    // since it's a genuinely separate deployable service, not just a
    // different message type on the same connection.
    constexpr const char* API_GATEWAY_HOST = "127.0.0.1";
    constexpr int API_GATEWAY_PORT = 9004;

    // Game Node's own liveness probe (MIGRATION_PLAN.md Phase 3) -- a
    // separate process from the WebSocket Gateway now, so it needs its own
    // health-check port; it has no client-facing port of its own otherwise
    // (all client traffic still enters through the Gateway's DEFAULT_PORT).
    constexpr int GAME_NODE_HEALTH_CHECK_PORT = 9005;
}

#endif
