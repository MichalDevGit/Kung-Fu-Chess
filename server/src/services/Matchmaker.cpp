#include "Matchmaker.h"

#include <algorithm>
#include <cstdlib>

#include "common/Config/MatchmakingConfig.h"

void Matchmaker::enqueue(const Entry& entry)
{
    std::lock_guard<std::mutex> lock(mutex);

    const bool alreadyQueued = std::any_of(queue.begin(), queue.end(), [&](const Entry& e)
        { return e.connectionId == entry.connectionId; });

    if (!alreadyQueued)
        queue.push_back(entry);
}

void Matchmaker::removeByConnection(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    queue.erase(
        std::remove_if(queue.begin(), queue.end(), [&](const Entry& e)
            { return e.connectionId == connectionId; }),
        queue.end());
}

bool Matchmaker::isQueued(const std::string& connectionId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    return std::any_of(queue.begin(), queue.end(), [&](const Entry& e)
        { return e.connectionId == connectionId; });
}

void Matchmaker::tick(
    long long nowMs,
    const std::function<void(const Match&)>& onMatched,
    const std::function<void(const Entry&)>& onTimedOut)
{
    std::vector<Match> matches;
    std::vector<Entry> timedOut;

    {
        std::lock_guard<std::mutex> lock(mutex);

        // Repeatedly pull the earliest-still-queued entry and look for the
        // closest-score partner still in the queue -- earliest overall is
        // therefore always match.first, satisfying "White = entered first"
        // without any extra bookkeeping.
        bool pairedSomething = true;
        while (pairedSomething)
        {
            pairedSomething = false;

            if (queue.size() < 2)
                break;

            const auto earliestIt = std::min_element(queue.begin(), queue.end(), [](const Entry& a, const Entry& b)
                { return a.enqueuedAtMs < b.enqueuedAtMs; });

            int bestScoreDiff = MatchmakingConfig::SCORE_RANGE + 1;
            auto bestIt = queue.end();

            for (auto it = queue.begin(); it != queue.end(); ++it)
            {
                if (it == earliestIt)
                    continue;

                const int scoreDiff = std::abs(it->score - earliestIt->score);
                if (scoreDiff <= MatchmakingConfig::SCORE_RANGE && scoreDiff < bestScoreDiff)
                {
                    bestScoreDiff = scoreDiff;
                    bestIt = it;
                }
            }

            if (bestIt != queue.end())
            {
                matches.push_back(Match{*earliestIt, *bestIt});

                // Erase the later index first so the earlier iterator/index
                // stays valid for the second erase.
                if (earliestIt < bestIt)
                {
                    queue.erase(bestIt);
                    queue.erase(earliestIt);
                }
                else
                {
                    queue.erase(earliestIt);
                    queue.erase(bestIt);
                }

                pairedSomething = true;
            }
        }

        queue.erase(
            std::remove_if(queue.begin(), queue.end(), [&](const Entry& e)
                {
                    if (nowMs - e.enqueuedAtMs < MatchmakingConfig::MAX_WAIT_MILLIS)
                        return false;
                    timedOut.push_back(e);
                    return true;
                }),
            queue.end());
    }

    for (const Match& match : matches)
        onMatched(match);

    for (const Entry& entry : timedOut)
        onTimedOut(entry);
}
