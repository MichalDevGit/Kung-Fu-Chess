#include "LocalMatchQueueStore.h"

#include <algorithm>

void LocalMatchQueueStore::add(const Entry& entry)
{
    std::lock_guard<std::mutex> lock(mutex);

    const bool alreadyQueued = std::any_of(queue.begin(), queue.end(), [&](const Entry& e)
        { return e.connectionId == entry.connectionId; });

    if (!alreadyQueued)
        queue.push_back(entry);
}

void LocalMatchQueueStore::remove(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    queue.erase(
        std::remove_if(queue.begin(), queue.end(), [&](const Entry& e)
            { return e.connectionId == connectionId; }),
        queue.end());
}

bool LocalMatchQueueStore::contains(const std::string& connectionId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    return std::any_of(queue.begin(), queue.end(), [&](const Entry& e)
        { return e.connectionId == connectionId; });
}

std::vector<Entry> LocalMatchQueueStore::all() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return queue;
}
