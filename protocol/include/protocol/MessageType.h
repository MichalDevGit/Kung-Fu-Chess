#ifndef PROTOCOL_MESSAGE_TYPE_H
#define PROTOCOL_MESSAGE_TYPE_H

namespace protocol
{
    // Wire values for the JSON envelope's "type" field. Shared by server and
    // client so both sides dispatch on the same literal strings.
    namespace MessageType
    {
        inline constexpr const char* Register = "register";
        inline constexpr const char* Login = "login";
        inline constexpr const char* RegisterResult = "register_result";
        inline constexpr const char* LoginResult = "login_result";
        inline constexpr const char* Error = "error";

        // Game session / gameplay messages.
        inline constexpr const char* JoinGame = "join_game";
        inline constexpr const char* GameJoined = "game_joined";
        inline constexpr const char* Move = "move";
        inline constexpr const char* Jump = "jump";
        inline constexpr const char* GameView = "game_view";
        inline constexpr const char* GameStarted = "game_started";
        inline constexpr const char* GameOver = "game_over";
    }
}

#endif
