#ifndef I_RECONNECT_RESOLVER_H
#define I_RECONNECT_RESOLVER_H

#include <optional>
#include <string>

#include "protocol/Message.h"

// Answers "does userId already have an active GameSession, and if so, rebind
// it to newConnectionId" -- the one check AuthRequestHandler needs on every
// login_token to support a reconnect (see completeLogin). Extracted as an
// interface in MIGRATION_PLAN.md Phase 3 specifically because
// GameSessionManager -- the thing that actually knows the answer -- moves
// into its own process (gamenode/) in this phase, while AuthRequestHandler
// stays in the WebSocket Gateway; LocalReconnectResolver (a direct in-process
// call, used by tests and by anyone still wiring these two classes together
// in one process) and RemoteReconnectResolver (a blocking Redis round trip to
// the real Game Node) are the two implementations.
//
// Returns a full protocol::MatchFoundResult (not GameSession::ResumeInfo)
// because that's exactly the shape AuthRequestHandler needs to hand to
// sendResume -- sessionId/color/opponentUsername/view -- and reusing it here
// means RemoteReconnectResolver can carry the answer across a process
// boundary with a struct that already has toJson()/fromJson().
class IReconnectResolver
{
public:
    virtual ~IReconnectResolver() = default;

    virtual std::optional<protocol::MatchFoundResult> checkAndRebind(
        int userId, const std::string& newConnectionId) = 0;
};

#endif
