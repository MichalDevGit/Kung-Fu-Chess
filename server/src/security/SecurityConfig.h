#ifndef SECURITY_CONFIG_H
#define SECURITY_CONFIG_H

// Single source of truth for security-related tuning, same pattern as
// common/Config/TimingConfig.h/MatchmakingConfig.h -- server-only (unlike
// those, nothing here is ever needed client-side), so it lives next to
// PasswordHasher instead of under common/Config.
namespace SecurityConfig
{
    // bcrypt work factor: the hash takes roughly 2^BCRYPT_COST_FACTOR
    // internal rounds, deliberately slow to resist brute force. 12 is
    // bcrypt's own recommended default as of the mid-2020s hardware baseline.
    constexpr int BCRYPT_COST_FACTOR = 12;
}

#endif
