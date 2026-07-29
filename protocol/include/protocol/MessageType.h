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

        // Sent over the WebSocket connection after a client has already
        // registered/logged in against the API Gateway's REST endpoints
        // (MIGRATION_PLAN.md Phase 2) and received a signed token back --
        // this is what actually binds connectionId -> user identity on this
        // connection now. Replies with the same "login_result" type/shape as
        // the (legacy, password-based) Login above.
        inline constexpr const char* LoginWithToken = "login_token";

        // Game session / gameplay messages. There is no more join_game/
        // game_joined -- a session now only ever comes into being via
        // matchmaking (see MatchFound below), which already hands the
        // client its initial GameView.
        inline constexpr const char* Move = "move";
        inline constexpr const char* Jump = "jump";
        inline constexpr const char* GameView = "game_view";
        inline constexpr const char* GameStarted = "game_started";
        inline constexpr const char* GameOver = "game_over";

        // Matchmaking messages. FindGame is sent by the client immediately
        // after a successful login; the outcome (MatchFound/NoMatch) is a
        // later, unsolicited push -- see server/src/services/Matchmaking/Matchmaker.
        inline constexpr const char* FindGame = "find_game";
        inline constexpr const char* Searching = "searching";
        inline constexpr const char* MatchFound = "match_found";
        inline constexpr const char* NoMatch = "no_match";
        inline constexpr const char* OpponentDisconnected = "opponent_disconnected";
        inline constexpr const char* OpponentReconnected = "opponent_reconnected";
    }
}

#endif
