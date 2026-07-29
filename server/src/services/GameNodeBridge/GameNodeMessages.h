#ifndef GAME_NODE_MESSAGES_H
#define GAME_NODE_MESSAGES_H

#include <string>

#include <nlohmann/json.hpp>

// The internal (never client-visible) envelope shapes carried over the two
// Redis pub/sub channels introduced in MIGRATION_PLAN.md Phase 3 -- kept
// out of protocol/ deliberately, since protocol/ is the client<->server wire
// contract, and no client ever sees these. Both GameNodeRequestPublisher/
// GameNodeRequestRouter (Gateway/Game Node ends of GameNodeConfig::
// REQUESTS_CHANNEL) and GameNodePushPublisher/GameNodePushRouter (the
// PUSHES_CHANNEL pair) share these two structs so there's one place that
// knows the field names.

// Gateway -> Game Node.
struct GameNodeRequest
{
    // "client_message"   -- forward rawJson (a find_game/move/jump request)
    //                       exactly as it arrived on connectionId.
    // "connection_closed" -- connectionId's socket just closed; rawJson/userId unused.
    // "reconnect_check"   -- does userId already have an active session? If
    //                       so, rebind it to connectionId. rawJson unused.
    std::string kind;
    std::string connectionId;
    std::string rawJson;
    int userId = 0;

    nlohmann::json toJson() const
    {
        return nlohmann::json{
            {"kind", kind}, {"connectionId", connectionId}, {"rawJson", rawJson}, {"userId", userId}};
    }

    static GameNodeRequest fromJson(const nlohmann::json& json)
    {
        return GameNodeRequest{
            json.at("kind").get<std::string>(),
            json.at("connectionId").get<std::string>(),
            json.value("rawJson", std::string()),
            json.value("userId", 0)};
    }
};

// Game Node -> Gateway.
struct GameNodePush
{
    // "push"                   -- json is a raw protocol/ message string to
    //                             relay to connectionId verbatim (via
    //                             WebSocketServer::sendTo), exactly as a
    //                             client would have received it from the
    //                             monolith.
    // "reconnect_check_result" -- json is a protocol::MatchFoundResult (see
    //                             LocalReconnectResolver) if connectionId's
    //                             user had an active session, empty
    //                             otherwise -- the reply to a
    //                             "reconnect_check" request.
    std::string kind;
    std::string connectionId;
    std::string json;

    nlohmann::json toJson() const
    {
        return nlohmann::json{{"kind", kind}, {"connectionId", connectionId}, {"json", json}};
    }

    static GameNodePush fromJson(const nlohmann::json& j)
    {
        return GameNodePush{
            j.at("kind").get<std::string>(), j.at("connectionId").get<std::string>(), j.value("json", std::string())};
    }
};

#endif
