#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include <nlohmann/json.hpp>
#include <string>

#include "protocol/MessageType.h"
#include "../../../common/DTO/GameView.h"
#include "../../../common/enums/EnumJson.h"
#include "../../../common/enums/PieceColor.h"

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

    // Sent over the WebSocket connection once the client already holds a
    // signed token from the API Gateway's REST /login (MIGRATION_PLAN.md
    // Phase 2) -- carries no password, since the WebSocket process never
    // verifies one directly anymore.
    struct LoginWithTokenRequest
    {
        std::string token;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::LoginWithToken}, {"token", token}};
            return j.dump();
        }

        static LoginWithTokenRequest fromJson(const nlohmann::json& j)
        {
            return LoginWithTokenRequest{j.at("token").get<std::string>()};
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
        // Only ever populated by the API Gateway's REST /login (see
        // MIGRATION_PLAN.md Phase 2) -- empty for the WebSocket process's
        // own login_result reply (it never issues tokens, only verifies
        // them). Kept on this same struct rather than a new one since the
        // gateway reuses LoginResult verbatim for its REST response body.
        std::string token;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::LoginResult}, {"success", success}};
            if (success)
            {
                j["userId"] = userId;
                j["score"] = score;
                if (!token.empty())
                    j["token"] = token;
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
            result.token = j.value("token", std::string());
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

    // reason/winnerUserId are both optional -- empty/0 for an ordinary
    // king-capture ending (the client already renders the board, so which
    // king is missing is visible without repeating it here); populated for
    // a disconnect-timeout forfeit (see GameSession::forfeitTo), the one
    // case where the losing side has no board evidence of why the game
    // ended.
    struct GameOverMessage
    {
        std::string reason;
        int winnerUserId = 0;

        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::GameOver}};
            if (!reason.empty())
                j["reason"] = reason;
            if (winnerUserId != 0)
                j["winnerUserId"] = winnerUserId;
            return j.dump();
        }

        static GameOverMessage fromJson(const nlohmann::json& j)
        {
            GameOverMessage message;
            message.reason = j.value("reason", std::string());
            message.winnerUserId = j.value("winnerUserId", 0);
            return message;
        }
    };

    // ---- Matchmaking messages ----

    // Sent by the client immediately after a successful login_result --
    // there is no manual "look for a game" step from the user's point of
    // view (see client/src/main.cpp).
    struct FindGameRequest
    {
        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::FindGame}};
            return j.dump();
        }

        static FindGameRequest fromJson(const nlohmann::json&)
        {
            return FindGameRequest{};
        }
    };

    // Synchronous ack for find_game -- the real outcome (MatchFoundResult/
    // NoMatchResult) arrives later as an unsolicited push once a second
    // player shows up or the wait times out, since matching is inherently
    // asynchronous.
    struct SearchingResult
    {
        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::Searching}};
            return j.dump();
        }

        static SearchingResult fromJson(const nlohmann::json&)
        {
            return SearchingResult{};
        }
    };

    struct MatchFoundResult
    {
        std::string sessionId;
        PieceColor color = PieceColor::None;
        std::string opponentUsername;
        GameView view;

        std::string toJson() const
        {
            nlohmann::json j{
                {"type", MessageType::MatchFound},
                {"sessionId", sessionId},
                {"color", color},
                {"opponentUsername", opponentUsername}};
            j["view"] = view.toJson();
            return j.dump();
        }

        static MatchFoundResult fromJson(const nlohmann::json& j)
        {
            return MatchFoundResult{
                j.value("sessionId", std::string()),
                j.value("color", PieceColor::None),
                j.value("opponentUsername", std::string()),
                GameView::fromJson(j.at("view"))};
        }
    };

    struct NoMatchResult
    {
        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::NoMatch}};
            return j.dump();
        }

        static NoMatchResult fromJson(const nlohmann::json&)
        {
            return NoMatchResult{};
        }
    };

    // Empty payloads -- only the "type" tag carries information, mirroring
    // GameStartedMessage/GameOverMessage above.
    struct OpponentDisconnectedMessage
    {
        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::OpponentDisconnected}};
            return j.dump();
        }

        static OpponentDisconnectedMessage fromJson(const nlohmann::json&)
        {
            return OpponentDisconnectedMessage{};
        }
    };

    struct OpponentReconnectedMessage
    {
        std::string toJson() const
        {
            nlohmann::json j{{"type", MessageType::OpponentReconnected}};
            return j.dump();
        }

        static OpponentReconnectedMessage fromJson(const nlohmann::json&)
        {
            return OpponentReconnectedMessage{};
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
