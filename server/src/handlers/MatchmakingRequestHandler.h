#ifndef MATCHMAKING_REQUEST_HANDLER_H
#define MATCHMAKING_REQUEST_HANDLER_H

#include <string>

class ConnectionRegistry;
class Matchmaker;
class ISessionIndexStore;

// Translates find_game into a Matchmaker::enqueue call. Mirrors AuthRequestHandler/
// GameRequestHandler's shape: knows the protocol/ message shapes, never throws.
// The actual match outcome is never this handler's synchronous reply -- see
// SearchingResult in protocol/Message.h -- it arrives later as a push once
// Matchmaker::tick pairs this connection or times it out (see gameallocator/
// src/main.cpp's runTickLoop). MIGRATION_PLAN.md Phase 4b: takes
// ISessionIndexStore& instead of a GameSessionManager& for its
// "already_in_game" guard, since this class now runs in gameallocator/'s own
// process, entirely separate from any Game Server Shard's GameSessionManager
// -- ISessionIndexStore is Redis-backed and already shared globally, so
// checking "does this connection already have a session, on any shard" only
// needs a read-only lookup against it, not a live session manager.
class MatchmakingRequestHandler
{
public:
    MatchmakingRequestHandler(ConnectionRegistry& connectionRegistry, Matchmaker& matchmaker, ISessionIndexStore& sessionIndexStore);

    std::string handle(const std::string& connectionId, const std::string& rawJson) const;

private:
    ConnectionRegistry& connectionRegistry;
    Matchmaker& matchmaker;
    ISessionIndexStore& sessionIndexStore;
};

#endif
