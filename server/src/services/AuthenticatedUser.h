#ifndef AUTHENTICATED_USER_H
#define AUTHENTICATED_USER_H

#include <string>

#include <nlohmann/json.hpp>

// The identity ConnectionRegistry binds to a live connection id. Hoisted out
// of ConnectionRegistry (which still exposes it as ConnectionRegistry::
// AuthenticatedUser via a using-declaration, so existing call sites are
// unaffected) so IConnectionStore's implementations can reference it without
// including the whole of ConnectionRegistry.h.
struct AuthenticatedUser
{
    int userId = 0;
    std::string username;
    int score = 0;

    // toJson/fromJson: only RedisConnectionStore needs these (to store a
    // value in Redis), same convention common/DTO already uses.
    nlohmann::json toJson() const
    {
        return nlohmann::json{{"userId", userId}, {"username", username}, {"score", score}};
    }

    static AuthenticatedUser fromJson(const nlohmann::json& json)
    {
        return AuthenticatedUser{json.at("userId").get<int>(), json.at("username").get<std::string>(), json.at("score").get<int>()};
    }
};

#endif
