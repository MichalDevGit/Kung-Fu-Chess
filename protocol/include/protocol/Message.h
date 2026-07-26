#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include <nlohmann/json.hpp>
#include <string>

#include "protocol/MessageType.h"
#include "../../../common/DTO/GameView.h"

// Header-only JSON message envelope shared by the server and every client
// (the CLI today, a future GUI client later) so both sides speak the exact
// same wire format from one definition. Each struct's toJson()/fromJson()
// pair is the only place that knows the JSON field names for that message.
namespace protocol
{
    struct RegisterRequest
    {
        std::string username;
        std::string password;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::Register}, {"username", username}, {"password", password}};
            return j.dump();
        }

        static RegisterRequest fromJson(const nlohmann::json& j)
        {
            return RegisterRequest{j.at("username").get<std::string>(), j.at("password").get<std::string>()};
        }
    };

    struct LoginRequest
    {
        std::string username;
        std::string password;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::Login}, {"username", username}, {"password", password}};
            return j.dump();
        }

        static LoginRequest fromJson(const nlohmann::json& j)
        {
            return LoginRequest{j.at("username").get<std::string>(), j.at("password").get<std::string>()};
        }
    };

    struct RegisterResult
    {
        bool success = false;
        int userId = 0;
        std::string error;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::RegisterResult}, {"success", success}};
            if (success)
                j["userId"] = userId;
            else
                j["error"] = error;
            return j.dump();
        }

        static RegisterResult fromJson(const nlohmann::json& j)
        {
            RegisterResult result;
            result.success = j.value("success", false);
            result.userId = j.value("userId", 0);
            result.error = j.value("error", std::string());
            return result;
        }
    };

    struct LoginResult
    {
        bool success = false;
        int userId = 0;
        int score = 0;
        std::string error;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::LoginResult}, {"success", success}};
            if (success)
            {
                j["userId"] = userId;
                j["score"] = score;
            }
            else
            {
                j["error"] = error;
            }
            return j.dump();
        }

        static LoginResult fromJson(const nlohmann::json& j)
        {
            LoginResult result;
            result.success = j.value("success", false);
            result.userId = j.value("userId", 0);
            result.score = j.value("score", 0);
            result.error = j.value("error", std::string());
            return result;
        }
    };

    struct ErrorResult
    {
        std::string error;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::Error}, {"error", error}};
            return j.dump();
        }

        static ErrorResult fromJson(const nlohmann::json& j)
        {
            return ErrorResult{j.value("error", std::string())};
        }
    };

    // ---- Game session / gameplay messages ----

    struct JoinGameRequest
    {
        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::JoinGame}};
            return j.dump();
        }

        static JoinGameRequest fromJson(const nlohmann::json&)
        {
            return JoinGameRequest{};
        }
    };

    // Carries the just-joined session's current GameView alongside the
    // sessionId -- join_game is a request/response message like any other, so
    // this is the only chance the server gets to hand the client an initial
    // board snapshot; without it the client has no view at all until its own
    // first move/jump request round-trips one back (and can't even select a
    // piece to make that first move, since selection is checked against the
    // last-received view).
    struct GameJoinedResult
    {
        std::string sessionId;
        GameView view;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::GameJoined}, {"sessionId", sessionId}};
            j["view"] = view.toJson();
            return j.dump();
        }

        static GameJoinedResult fromJson(const nlohmann::json& j)
        {
            return GameJoinedResult{j.value("sessionId", std::string()), GameView::fromJson(j.at("view"))};
        }
    };

    struct MoveRequest
    {
        int fromRow = 0;
        int fromCol = 0;
        int toRow = 0;
        int toCol = 0;

        std::string toJson() const
        {
            nlohmann::json j{
                {"type", MessageType::Move},
                {"fromRow", fromRow},
                {"fromCol", fromCol},
                {"toRow", toRow},
                {"toCol", toCol}};
            return j.dump();
        }

        static MoveRequest fromJson(const nlohmann::json& j)
        {
            return MoveRequest{
                j.at("fromRow").get<int>(),
                j.at("fromCol").get<int>(),
                j.at("toRow").get<int>(),
                j.at("toCol").get<int>()};
        }
    };

    struct JumpRequest
    {
        int row = 0;
        int col = 0;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::Jump}, {"row", row}, {"col", col}};
            return j.dump();
        }

        static JumpRequest fromJson(const nlohmann::json& j)
        {
            return JumpRequest{j.at("row").get<int>(), j.at("col").get<int>()};
        }
    };

    // Wraps the common::GameView per-frame snapshot DTO -- the server pushes
    // one of these to every connection subscribed to a GameSession whenever
    // an EventBus event fires (see GameSession, added in a later phase).
    struct GameViewMessage
    {
        GameView view;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::GameView}};
            j["view"] = view.toJson();
            return j.dump();
        }

        static GameViewMessage fromJson(const nlohmann::json& j)
        {
            return GameViewMessage{GameView::fromJson(j.at("view"))};
        }
    };

    // Thin wrappers mirroring common::GameStartedEvent/GameOverEvent -- both
    // are empty payloads, so only the "type" tag carries information.
    struct GameStartedMessage
    {
        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::GameStarted}};
            return j.dump();
        }

        static GameStartedMessage fromJson(const nlohmann::json&)
        {
            return GameStartedMessage{};
        }
    };

    struct GameOverMessage
    {
        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::GameOver}};
            return j.dump();
        }

        static GameOverMessage fromJson(const nlohmann::json&)
        {
            return GameOverMessage{};
        }
    };

    // Reads the top-level "type" field. Throws nlohmann::json::exception on a
    // missing field -- callers should already be inside a try/catch around the
    // whole decode step (parsing the raw string can throw the same way).
    inline std::string readType(const nlohmann::json& j)
    {
        return j.at("type").get<std::string>();
    }
}

#endif
