#ifndef COMMON_CONFIG_GAME_NODE_CONFIG_H
#define COMMON_CONFIG_GAME_NODE_CONFIG_H

// Single source of truth for the Gateway<->Game Node boundary introduced in
// MIGRATION_PLAN.md Phase 3 -- shared by server/ (the WebSocket Gateway,
// publishes requests / subscribes pushes) and gamenode/ (the reverse), same
// pattern as TimingConfig.h/MatchmakingConfig.h. Both processes must agree on
// these channel names since they're just two ends of the same Redis pub/sub
// pipe (KUNGFUCHESS_REDIS_URL, mandatory for a real split -- see
// ARCHITECTURE.md's Known gaps).
namespace GameNodeConfig
{
    // Gateway -> Game Node: forwarded client requests (find_game/move/jump),
    // connection-closed notifications, and login-reconnect-check requests --
    // see GameNodeBridge/GameNodeMessages.h for the envelope shape.
    inline constexpr const char* REQUESTS_CHANNEL = "gamenode:requests";

    // Game Node -> Gateway: every push destined for a specific connectionId
    // (game_view/match_found/no_match/game_over/opponent_disconnected/
    // opponent_reconnected, and reconnect-check replies).
    inline constexpr const char* PUSHES_CHANNEL = "gateway:pushes";

    // How long AuthRequestHandler's login path blocks waiting for the Game
    // Node's answer to "does this user already have an active session"
    // before giving up and treating it as "no session found" -- this is the
    // one place Phase 3 keeps a synchronous round trip (see
    // RemoteReconnectResolver), since CliShell relies on the resume push
    // arriving before login_result.
    constexpr long long RECONNECT_CHECK_TIMEOUT_MILLIS = 3000;
}

#endif
