// API Gateway entry point (MIGRATION_PLAN.md Phase 2): a small, separately
// deployable HTTP service that owns register/login against the user store,
// issuing a signed token on a successful login instead of the WebSocket
// process ever verifying a password itself. Reuses AuthService/
// RepositoryFactory verbatim from server/ (this executable links
// KungFuChessAuth) -- the only new logic here is ApiGatewayRequestHandler
// (REST body <-> AuthService translation) and TokenService (common/).
#include <cstdlib>
#include <memory>
#include <string>

#include "ApiGatewayRequestHandler.h"
#include "network/ApiGatewayServer.h"
#include "persistence/IUserRepository.h"
#include "persistence/RepositoryFactory.h"
#include "services/AuthService.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/TokenConfig.h"
#include "common/Logging/Logger.h"
#include "common/Security/TokenService.h"

int main()
{
    const char* hostOverride = std::getenv("KUNGFUCHESS_HOST");
    const std::string bindHost = hostOverride ? std::string(hostOverride) : NetworkConfig::API_GATEWAY_HOST;

    // Same backend-selection pattern as server/src/main.cpp: SQLite is the
    // actual default, KUNGFUCHESS_POSTGRES_URL opts into Postgres. Once this
    // and KungFuChessServer are genuinely separate processes, both MUST be
    // pointed at the same backend (see ARCHITECTURE.md's Known gaps) --
    // docker-compose.yml sets the same KUNGFUCHESS_POSTGRES_URL on both
    // services for exactly this reason.
    const std::string dbPath = "kungfuchess.db";
    const char* postgresUrl = std::getenv("KUNGFUCHESS_POSTGRES_URL");
    std::unique_ptr<IUserRepository> users = postgresUrl
        ? RepositoryFactory::createUserRepository(RepositoryBackend::Postgres, "", postgresUrl)
        : RepositoryFactory::createUserRepository(RepositoryBackend::Sqlite, dbPath);
    AuthService authService(*users);

    // KUNGFUCHESS_TOKEN_SECRET must match what server/'s WebSocket process
    // is configured with -- the dev default only works because both
    // processes fall back to the exact same constant when neither sets it.
    const char* tokenSecretEnv = std::getenv("KUNGFUCHESS_TOKEN_SECRET");
    const std::string tokenSecret = tokenSecretEnv ? std::string(tokenSecretEnv) : TokenConfig::DEV_INSECURE_DEFAULT_SECRET;
    security::TokenService tokenService(tokenSecret);

    ApiGatewayRequestHandler handler(authService, tokenService);
    ApiGatewayServer server(handler, NetworkConfig::API_GATEWAY_PORT, bindHost);

    server.start();
    common::Logger::info(
        "KungFuChess API Gateway listening on http://" + bindHost + ":" + std::to_string(NetworkConfig::API_GATEWAY_PORT));
    server.wait();

    return 0;
}
