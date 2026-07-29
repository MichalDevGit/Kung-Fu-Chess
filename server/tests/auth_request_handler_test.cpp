#include "tests/doctest.h"

#include <nlohmann/json.hpp>

#include "handlers/AuthRequestHandler.h"
#include "common/Config/TokenConfig.h"
#include "common/Security/TokenService.h"
#include "common/WallClock.h"
#include "persistence/InMemory/InMemoryUserRepository.h"
#include "services/Connection/ConnectionRegistry.h"
#include "services/GameNodeBridge/LocalReconnectResolver.h"
#include "services/GameSession/GameSessionManager.h"
#include "services/Connection/LocalConnectionStore.h"
#include "services/SessionIndex/LocalSessionIndexStore.h"

namespace
{
struct Fixture
{
    InMemoryUserRepository repo;
    security::TokenService tokenService{"test-secret"};
    LocalConnectionStore connectionStore;
    ConnectionRegistry connectionRegistry{connectionStore};
    LocalSessionIndexStore sessionIndexStore;
    GameSessionManager sessionManager{[](const std::string&, const std::string&) {}, repo, sessionIndexStore};
    LocalReconnectResolver reconnectResolver{sessionManager};
    AuthRequestHandler handler{
        repo, tokenService, connectionRegistry, reconnectResolver, [](const std::string&, const std::string&) {}};
};
}

TEST_CASE("AuthRequestHandler login_token")
{
    Fixture fixture;
    fixture.repo.createUser("alice", "irrelevant-hash-this-handler-never-reads-it");

    SUBCASE("a valid token binds the connection and returns a successful login_result")
    {
        // handle() verifies against real wallClockMillis() internally, so a
        // "currently valid" token must be issued relative to that same
        // clock, not an arbitrary epoch like 0.
        const std::string token = fixture.tokenService.issue(1, wallClockMillis());

        nlohmann::json request{{"type", "login_token"}, {"token", token}};
        nlohmann::json response = nlohmann::json::parse(fixture.handler.handle("conn1", request.dump()));

        CHECK(response.at("type").get<std::string>() == "login_result");
        CHECK(response.at("success").get<bool>());
        CHECK(response.at("userId").get<int>() == 1);
        CHECK_FALSE(response.contains("token"));

        const std::optional<ConnectionRegistry::AuthenticatedUser> bound = fixture.connectionRegistry.find("conn1");
        REQUIRE(bound.has_value());
        CHECK(bound->userId == 1);
        CHECK(bound->username == "alice");
    }

    SUBCASE("an invalid signature is rejected without binding the connection")
    {
        nlohmann::json request{{"type", "login_token"}, {"token", "not-a-real-token"}};
        nlohmann::json response = nlohmann::json::parse(fixture.handler.handle("conn1", request.dump()));

        CHECK(response.at("type").get<std::string>() == "login_result");
        CHECK_FALSE(response.at("success").get<bool>());
        CHECK_FALSE(fixture.connectionRegistry.find("conn1").has_value());
    }

    SUBCASE("an expired token is rejected")
    {
        // handle() verifies against wallClockMillis() (real "now") internally,
        // so forge a token issued far enough in the past (relative to the
        // Unix epoch) that TOKEN_TTL_MILLIS has already elapsed by the time
        // this runs.
        const std::string expiredToken = fixture.tokenService.issue(1, -(TokenConfig::TOKEN_TTL_MILLIS + 10000));
        nlohmann::json expiredRequest{{"type", "login_token"}, {"token", expiredToken}};
        nlohmann::json response = nlohmann::json::parse(fixture.handler.handle("conn1", expiredRequest.dump()));

        CHECK(response.at("type").get<std::string>() == "login_result");
        CHECK_FALSE(response.at("success").get<bool>());
        CHECK(response.at("error").get<std::string>() == "expired");
    }

    SUBCASE("a token for an unknown userId is rejected")
    {
        const std::string token = fixture.tokenService.issue(999, wallClockMillis());
        nlohmann::json request{{"type", "login_token"}, {"token", token}};
        nlohmann::json response = nlohmann::json::parse(fixture.handler.handle("conn1", request.dump()));

        CHECK(response.at("type").get<std::string>() == "login_result");
        CHECK_FALSE(response.at("success").get<bool>());
        CHECK(response.at("error").get<std::string>() == "unknown_user");
    }
}

TEST_CASE("AuthRequestHandler no longer recognizes the legacy password-based register/login "
          "(MIGRATION_PLAN.md Phase 2, step B -- those now live only in apigateway/)")
{
    Fixture fixture;

    nlohmann::json registerRequest{{"type", "register"}, {"username", "bob"}, {"password", "pw"}};
    nlohmann::json registerResponse = nlohmann::json::parse(fixture.handler.handle("conn1", registerRequest.dump()));
    CHECK(registerResponse.at("type").get<std::string>() == "error");
    CHECK(registerResponse.at("error").get<std::string>() == "unknown_type");

    nlohmann::json loginRequest{{"type", "login"}, {"username", "bob"}, {"password", "pw"}};
    nlohmann::json loginResponse = nlohmann::json::parse(fixture.handler.handle("conn1", loginRequest.dump()));
    CHECK(loginResponse.at("type").get<std::string>() == "error");
    CHECK(loginResponse.at("error").get<std::string>() == "unknown_type");
}
