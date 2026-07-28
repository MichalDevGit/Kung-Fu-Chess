#ifndef COMMON_CONFIG_TOKEN_CONFIG_H
#define COMMON_CONFIG_TOKEN_CONFIG_H

// Single source of truth for login-token tuning, same pattern as
// TimingConfig.h/MatchmakingConfig.h -- shared by server/'s WebSocket
// process (verifies tokens) and apigateway/ (issues them), which is exactly
// why this lives under common/Config rather than server-only like
// server/src/security/SecurityConfig.h.
namespace TokenConfig
{
    // How long an issued token stays valid.
    constexpr long long TOKEN_TTL_MILLIS = 24LL * 60 * 60 * 1000; // 24 hours

    // Used only when the KUNGFUCHESS_TOKEN_SECRET environment variable is
    // unset, so a native/local run with no configuration still works --
    // mirrors KUNGFUCHESS_POSTGRES_URL/KUNGFUCHESS_REDIS_URL being opt-in.
    // Unlike those, this one is a real security gap if left in place for
    // anything beyond local development: both the apigateway and server
    // processes must be given the same real secret via that environment
    // variable for any deployment where token forgery would matter (see
    // ARCHITECTURE.md's Known gaps).
    inline constexpr const char* DEV_INSECURE_DEFAULT_SECRET = "kungfuchess-dev-insecure-default-secret";
}

#endif
