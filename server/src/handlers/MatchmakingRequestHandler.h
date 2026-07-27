#ifndef MATCHMAKING_REQUEST_HANDLER_H
#define MATCHMAKING_REQUEST_HANDLER_H

#include <string>

class ConnectionRegistry;
class Matchmaker;
class GameSessionManager;

// Translates find_game into a Matchmaker::enqueue call. Mirrors AuthRequestHandler/
// GameRequestHandler's shape: knows the protocol/ message shapes, never throws.
// The actual match outcome is never this handler's synchronous reply -- see
// SearchingResult in protocol/Message.h -- it arrives later as a push once
// Matchmaker::tick pairs this connection or times it out (server/main.cpp).
class MatchmakingRequestHandler
{
public:
    MatchmakingRequestHandler(ConnectionRegistry& connectionRegistry, Matchmaker& matchmaker, GameSessionManager& sessionManager);

    std::string handle(const std::string& connectionId, const std::string& rawJson) const;

private:
    ConnectionRegistry& connectionRegistry;
    Matchmaker& matchmaker;
    GameSessionManager& sessionManager;
};

#endif
