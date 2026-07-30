#include "tests/doctest.h"
#include "handlers/GameRequestHandler.h"
#include "services/GameSession/GameSessionManager.h"
#include "services/SessionIndex/LocalSessionIndexStore.h"
#include "persistence/InMemory/InMemoryUserRepository.h"

#include <nlohmann/json.hpp>
#include <string>

namespace
{
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
    InMemoryUserRepository repo;
    LocalSessionIndexStore indexStore;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo, indexStore);
    manager.createSession("test-session", whitePlayer(), blackPlayer());
    GameRequestHandler handler(manager);

    nlohmann::json request{
        {"type", "move"}, {"fromRow", 6}, {"fromCol", 4}, {"toRow", 5}, {"toCol", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-white", request.dump()));

    CHECK(response.at("type").get<std::string>() == "game_view");
    CHECK(response.at("view").contains("board"));
}

TEST_CASE("GameRequestHandler rejects a move for the opponent's piece")
{
    InMemoryUserRepository repo;
    LocalSessionIndexStore indexStore;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo, indexStore);
    manager.createSession("test-session", whitePlayer(), blackPlayer());
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
    InMemoryUserRepository repo;
    LocalSessionIndexStore indexStore;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo, indexStore);
    manager.createSession("test-session", whitePlayer(), blackPlayer());
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "jump"}, {"row", 6}, {"col", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-white", request.dump()));

    CHECK(response.at("type").get<std::string>() == "game_view");
}

TEST_CASE("GameRequestHandler returns an error when the connection has no active session")
{
    InMemoryUserRepository repo;
    LocalSessionIndexStore indexStore;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo, indexStore);
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "jump"}, {"row", 6}, {"col", 4}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-nobody", request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "no_active_game");
}

TEST_CASE("GameRequestHandler returns an error for an unknown message type")
{
    InMemoryUserRepository repo;
    LocalSessionIndexStore indexStore;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo, indexStore);
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "not_a_real_type"}};
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn1", request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "unknown_type");
}

TEST_CASE("GameRequestHandler returns an error for malformed JSON")
{
    InMemoryUserRepository repo;
    LocalSessionIndexStore indexStore;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo, indexStore);
    GameRequestHandler handler(manager);

    nlohmann::json response = nlohmann::json::parse(handler.handle("conn1", "{ this is not valid json"));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "malformed_request");
}

TEST_CASE("GameRequestHandler returns an error when a required field is missing")
{
    InMemoryUserRepository repo;
    LocalSessionIndexStore indexStore;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo, indexStore);
    manager.createSession("test-session", whitePlayer(), blackPlayer());
    GameRequestHandler handler(manager);

    nlohmann::json request{{"type", "move"}, {"fromRow", 6}}; // missing fromCol/toRow/toCol
    nlohmann::json response = nlohmann::json::parse(handler.handle("conn-white", request.dump()));

    CHECK(response.at("type").get<std::string>() == "error");
    CHECK(response.at("error").get<std::string>() == "malformed_request");
}
