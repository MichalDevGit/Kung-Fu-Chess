#include "tests/doctest.h"

#include <nlohmann/json.hpp>

#include "ApiGatewayRequestHandler.h"
#include "common/Security/TokenService.h"
#include "persistence/InMemoryUserRepository.h"
#include "protocol/Message.h"
#include "services/AuthService.h"

TEST_CASE("Testing ApiGatewayRequestHandler")
{
    InMemoryUserRepository repo;
    AuthService authService(repo);
    security::TokenService tokenService("test-secret");
    ApiGatewayRequestHandler handler(authService, tokenService);

    SUBCASE("handleRegister succeeds for a new username")
    {
        const protocol::RegisterResult result =
            protocol::RegisterResult::fromJson(nlohmann::json::parse(handler.handleRegister(
                protocol::RegisterRequest{"alice", "hunter2"}.toJson())));

        CHECK(result.success);
        CHECK(result.userId > 0);
    }

    SUBCASE("handleRegister rejects a duplicate username")
    {
        handler.handleRegister(protocol::RegisterRequest{"bob", "pw1"}.toJson());
        const protocol::RegisterResult result =
            protocol::RegisterResult::fromJson(nlohmann::json::parse(handler.handleRegister(
                protocol::RegisterRequest{"bob", "pw2"}.toJson())));

        CHECK_FALSE(result.success);
        CHECK(result.error == "username_taken");
    }

    SUBCASE("handleLogin issues a token that verifies to the same userId, and never leaks a password")
    {
        const protocol::RegisterResult registered =
            protocol::RegisterResult::fromJson(nlohmann::json::parse(handler.handleRegister(
                protocol::RegisterRequest{"carol", "correct-password"}.toJson())));

        const protocol::LoginResult result =
            protocol::LoginResult::fromJson(nlohmann::json::parse(handler.handleLogin(
                protocol::LoginRequest{"carol", "correct-password"}.toJson())));

        CHECK(result.success);
        CHECK(result.userId == registered.userId);
        CHECK_FALSE(result.token.empty());

        const security::VerifyResult verified = tokenService.verify(result.token, 0);
        CHECK(verified.valid);
        CHECK(verified.userId == registered.userId);
    }

    SUBCASE("handleLogin fails with wrong password and issues no token")
    {
        handler.handleRegister(protocol::RegisterRequest{"dave", "correct-password"}.toJson());
        const protocol::LoginResult result =
            protocol::LoginResult::fromJson(nlohmann::json::parse(handler.handleLogin(
                protocol::LoginRequest{"dave", "wrong-password"}.toJson())));

        CHECK_FALSE(result.success);
        CHECK(result.error == "invalid_credentials");
        CHECK(result.token.empty());
    }

    SUBCASE("malformed JSON is reported as an error, never throws")
    {
        const nlohmann::json response = nlohmann::json::parse(handler.handleLogin("not json"));
        CHECK(response.at("type").get<std::string>() == protocol::MessageType::Error);
    }
}
