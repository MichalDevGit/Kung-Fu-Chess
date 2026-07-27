#include "tests/doctest.h"
#include "handlers/GameRequestHandler.h"
#include "services/GameSessionManager.h"
#include "persistence/Database.h"
#include "persistence/UserRepository.h"

#include <nlohmann/json.hpp>
#include <string>

namespace
{
struct TestUserRepository
{
    Database database{":memory:"};
    UserRepository users{database};
};

GameSession::Player whitePlayer()
{
    return GameSession::Player{1, "alice", "conn-white"};
}

GameSession::Player blackPlayer()
{
    return GameSession::Player{2, "bob", "conn-black"};
}
}

TEST_CASE("GameRequestHandler handles move for the requester's own piece and returns a fresh GameView snapshot")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);
    manager.createSession(whitePlayer(), blackPlayer());
    GameRequestHandler handler(manager);

    nlohmann::json request{
        {"type", "move"}, {"fromRow", 6}, {"fromCol", 4}, {"toRow", 5}, {"toCol", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-white", request.dump()));

    CHECK(response.at("type").get<std::string>() == "game_view");
    CHECK(response.at("view").contains("board"));
}

TEST_CASE("GameRequestHandler rejects a move for the opponent's piece")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);
    manager.createSession(whitePlayer(), blackPlayer());
    GameRequestHandler handler(manager);

    // Row 1 is a black pawn on the classic starting board -- white's
    // connection has no business moving it.
    nlohmann::json request{
        {"type", "move"}, {"fromRow", 1}, {"fromCol", 4}, {"toRow", 2}, {"toCol", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-white", request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "not_your_piece");
}

TEST_CASE("GameRequestHandler handles jump and returns a fresh GameView snapshot")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);
    manager.createSession(whitePlayer(), blackPlayer());
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "jump"}, {"row", 6}, {"col", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-white", request.dump()));

    CHECK(response.at("type").get<std::string>() == "game_view");
}

TEST_CASE("GameRequestHandler returns an error when the connection has no active session")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "jump"}, {"row", 6}, {"col", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-nobody", request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "no_active_game");
}

TEST_CASE("GameRequestHandler returns an error for an unknown message type")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "not_a_real_type"}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn1", request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "unknown_type");
}

TEST_CASE("GameRequestHandler returns an error for malformed JSON")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);
    GameRequestHandler handler(manager);

    nlohmann::json response = nlohmann::json::parse(handler.handle("conn1", "{ this is not valid json"));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "malformed_request");
}

TEST_CASE("GameRequestHandler returns an error when a required field is missing")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);
    manager.createSession(whitePlayer(), blackPlayer());
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "move"}, {"fromRow", 6}}; // missing fromCol/toRow/toCol
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-white", request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "malformed_request");
}
