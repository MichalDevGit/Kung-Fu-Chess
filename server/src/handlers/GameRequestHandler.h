#ifndef GAME_REQUEST_HANDLER_H
#define GAME_REQUEST_HANDLER_H

#include <string>

class GameSessionManager;

// Translates a raw JSON game-command request (join_game/move/jump) into a
// GameSessionManager/GameSession call and a raw JSON response string.
// Mirrors AuthRequestHandler's shape: knows the protocol/ message shapes,
// knows nothing about ixwebsocket or sockets, never throws.
class GameRequestHandler
{
public:
    explicit GameRequestHandler(GameSessionManager& sessionManager);

    // Never throws -- any parse/validation failure is caught internally and
    // turned into a protocol::ErrorResult envelope instead.
    std::string handle(const std::string& rawJson) const;

private:
    GameSessionManager& sessionManager;
};

#endif
