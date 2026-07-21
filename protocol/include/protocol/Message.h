#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include <nlohmann/json.hpp>
#include <string>

#include "protocol/MessageType.h"

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

    // Reads the top-level "type" field. Throws nlohmann::json::exception on a
    // missing field -- callers should already be inside a try/catch around the
    // whole decode step (parsing the raw string can throw the same way).
    inline std::string readType(const nlohmann::json& j)
    {
        return j.at("type").get<std::string>();
    }
}

#endif
