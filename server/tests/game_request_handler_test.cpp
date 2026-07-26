#include "tests/doctest.h"
#include "handlers/GameRequestHandler.h"
#include "services/GameSessionManager.h"

#include <nlohmann/json.hpp>
#include <string>

namespace
{
GameSessionManager makeManager()
{
    return GameSessionManager([](const std::string&) {});
}
}

TEST_CASE("GameRequestHandler handles join_game")
{
    GameSessionManager manager = makeManager();
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "join_game"}};
    nlohmann::json response = nlohmann::json::parse(handler.handle(request.dump()));

    CHECK(response.at("type").get<std::string>() == "game_joined");
    CHECK(response.at("sessionId").get<std::string>() == "default");
}

TEST_CASE("GameRequestHandler handles move and returns a fresh GameView snapshot")
{
    GameSessionManager manager = makeManager();
    GameRequestHandler handler(manager);

    nlohmann::json request{
        {"type", "move"}, {"fromRow", 6}, {"fromCol", 4}, {"toRow", 5}, {"toCol", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle(request.dump()));

    CHECK(response.at("type").get<std::string>() == "game_view");
    CHECK(response.at("view").contains("board"));
}

TEST_CASE("GameRequestHandler handles jump and returns a fresh GameView snapshot")
{
    GameSessionManager manager = makeManager();
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "jump"}, {"row", 6}, {"col", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle(request.dump()));

    CHECK(response.at("type").get<std::string>() == "game_view");
}

TEST_CASE("GameRequestHandler returns an error for an unknown message type")
{
    GameSessionManager manager = makeManager();
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "not_a_real_type"}};
    nlohmann::json response = nlohmann::json::parse(handler.handle(request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "unknown_type");
}

TEST_CASE("GameRequestHandler returns an error for malformed JSON")
{
    GameSessionManager manager = makeManager();
    GameRequestHandler handler(manager);

    nlohmann::json response = nlohmann::json::parse(handler.handle("{ this is not valid json"));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "malformed_request");
}

TEST_CASE("GameRequestHandler returns an error when a required field is missing")
{
    GameSessionManager manager = makeManager();
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "move"}, {"fromRow", 6}}; // missing fromCol/toRow/toCol
    nlohmann::json response = nlohmann::json::parse(handler.handle(request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "malformed_request");
}
