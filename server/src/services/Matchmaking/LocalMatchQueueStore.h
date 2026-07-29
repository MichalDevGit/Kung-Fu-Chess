#ifndef LOCAL_MATCH_QUEUE_STORE_H
#define LOCAL_MATCH_QUEUE_STORE_H

#include <mutex>
#include <vector>

#include "IMatchQueueStore.h"

// In-memory IMatchQueueStore -- today's default. Ports the vector<Entry> +
// mutex Matchmaker used to own directly, unchanged.
class LocalMatchQueueStore : public IMatchQueueStore
{
public:
    void add(const Entry& entry) override;
    void remove(const std::string& connectionId) override;
    bool contains(const std::string& connectionId) const override;
    std::vector<Entry> all() const override;

private:
    mutable std::mutex mutex;
    std::vector<Entry> queue;
};

#endif
