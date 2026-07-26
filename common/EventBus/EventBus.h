#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

// Generic, type-safe publish/subscribe bus. Knows nothing about any specific
// event type -- routing is keyed by the C++ type of the event struct passed to
// subscribe<Event>()/publish<Event>(), so adding a new event type never
// requires changing this class.
//
// Thread safety: subscribe/unsubscribe/publish all lock the same mutex, but
// publish only holds it long enough to copy out the matching handler list --
// the handlers themselves run unlocked, on the calling thread, after the lock
// is released. This keeps a handler that itself calls subscribe/unsubscribe/
// publish from deadlocking, and matches the rest of this codebase's plain-
// std::mutex style (see server/src/services/AuthService) rather than a
// queued/dispatched model, since nothing here needs deferred delivery.
class EventBus
{
public:
    using SubscriptionId = std::size_t;

    template <typename Event>
    SubscriptionId subscribe(std::function<void(const Event&)> handler)
    {
        SubscriptionId id = nextId.fetch_add(1);

        std::lock_guard<std::mutex> lock(mutex);

        subscriberListFor<Event>().handlers.emplace_back(id, std::move(handler));

        return id;
    }

    template <typename Event>
    void unsubscribe(SubscriptionId id)
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = subscribersByType.find(std::type_index(typeid(Event)));

        if (it == subscribersByType.end())
        {
            return;
        }

        auto& handlers = static_cast<SubscriberList<Event>&>(*it->second).handlers;

        handlers.erase(
            std::remove_if(
                handlers.begin(),
                handlers.end(),
                [id](const auto& entry) { return entry.first == id; }),
            handlers.end());
    }

    template <typename Event>
    void publish(const Event& event) const
    {
        std::vector<std::function<void(const Event&)>> handlersSnapshot;

        {
            std::lock_guard<std::mutex> lock(mutex);

            auto it = subscribersByType.find(std::type_index(typeid(Event)));

            if (it == subscribersByType.end())
            {
                return;
            }

            const auto& handlers = static_cast<const SubscriberList<Event>&>(*it->second).handlers;

            handlersSnapshot.reserve(handlers.size());

            for (const auto& entry : handlers)
            {
                handlersSnapshot.push_back(entry.second);
            }
        }

        for (const auto& handler : handlersSnapshot)
        {
            handler(event);
        }
    }

private:
    struct ISubscriberList
    {
        virtual ~ISubscriberList() = default;
    };

    template <typename Event>
    struct SubscriberList : ISubscriberList
    {
        std::vector<std::pair<SubscriptionId, std::function<void(const Event&)>>> handlers;
    };

    template <typename Event>
    SubscriberList<Event>& subscriberListFor()
    {
        auto& entry = subscribersByType[std::type_index(typeid(Event))];

        if (!entry)
        {
            entry = std::make_unique<SubscriberList<Event>>();
        }

        return static_cast<SubscriberList<Event>&>(*entry);
    }

    mutable std::mutex mutex;
    std::atomic<SubscriptionId> nextId{0};
    std::unordered_map<std::type_index, std::unique_ptr<ISubscriberList>> subscribersByType;
};

#endif
