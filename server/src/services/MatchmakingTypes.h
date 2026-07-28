#ifndef MATCHMAKING_TYPES_H
#define MATCHMAKING_TYPES_H

#include <string>

#include <nlohmann/json.hpp>

// Hoisted out of Matchmaker (which still exposes both as Matchmaker::Entry/
// Matchmaker::Match via using-declarations, so existing call sites are
// unaffected) so IMatchQueueStore's implementations can reference them
// without including the whole of Matchmaker.h.

// A queued, authenticated connection waiting to be paired.
struct Entry
{
    std::string connectionId;
    int userId = 0;
    std::string username;
    int score = 0;
    long long enqueuedAtMs = 0;

    // toJson/fromJson: only RedisMatchQueueStore needs these (to store a
    // value in Redis), same convention common/DTO already uses.
    nlohmann::json toJson() const
    {
        return nlohmann::json{
            {"connectionId", connectionId}, {"userId", userId}, {"username", username}, {"score", score}, {"enqueuedAtMs", enqueuedAtMs}};
    }

    static Entry fromJson(const nlohmann::json& json)
    {
        return Entry{
            json.at("connectionId").get<std::string>(),
            json.at("userId").get<int>(),
            json.at("username").get<std::string>(),
            json.at("score").get<int>(),
            json.at("enqueuedAtMs").get<long long>()};
    }
};

// Two matched entries -- first is always the earlier-enqueued of the pair,
// so a caller assigning White = first-queued never has to re-derive
// ordering itself.
struct Match
{
    Entry first;
    Entry second;
};

#endif
