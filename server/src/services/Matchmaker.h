#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <functional>
#include <mutex>
#include <string>
#include <vector>

// Pairs up queued, authenticated connections into 1v1 matches by score
// proximity and queue order. Pure in-memory bookkeeping -- no networking
// types, no game engine -- so it's driven by whatever already has a tick
// (the server's existing tick thread, see main.cpp's runTickLoop) rather
// than owning a thread of its own.
class Matchmaker
{
public:
    struct Entry
    {
        std::string connectionId;
        int userId = 0;
        std::string username;
        int score = 0;
        long long enqueuedAtMs = 0;
    };

    // Two matched entries -- first is always the earlier-enqueued of the
    // pair, so a caller assigning White = first-queued never has to
    // re-derive ordering itself.
    struct Match
    {
        Entry first;
        Entry second;
    };

    // No-op if this connectionId is already queued (a duplicate find_game
    // from the same connection shouldn't reset its place in line or create
    // a second entry).
    void enqueue(const Entry& entry);

    // Removes a queued entry for this connection, if any -- called on
    // disconnect while still waiting, or when a fresh login supersedes an
    // older connection for the same user.
    void removeByConnection(const std::string& connectionId);

    bool isQueued(const std::string& connectionId) const;

    // Scans the queue once: pairs the earliest-queued entry with the
    // closest-score entry still in the queue within MatchmakingConfig::
    // SCORE_RANGE (removing both from the queue), for every pairing
    // possible this tick; then removes and reports every remaining entry
    // that has waited longer than MatchmakingConfig::MAX_WAIT_MILLIS.
    void tick(
        long long nowMs,
        const std::function<void(const Match&)>& onMatched,
        const std::function<void(const Entry&)>& onTimedOut);

private:
    mutable std::mutex mutex;
    std::vector<Entry> queue;
};

#endif
