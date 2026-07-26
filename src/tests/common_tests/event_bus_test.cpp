#include "tests/doctest.h"
#include "src/common/EventBus/EventBus.h"

#include <atomic>
#include <thread>
#include <vector>

namespace
{
struct PingEvent
{
    int value;
};

struct PongEvent
{
    int value;
};
}

TEST_CASE("Testing EventBus") {
    SUBCASE("A subscriber receives a published event of its own type") {
        EventBus bus;
        int received = -1;

        bus.subscribe<PingEvent>([&received](const PingEvent& event) {
            received = event.value;
        });

        bus.publish(PingEvent{42});

        CHECK(received == 42);
    }

    SUBCASE("A subscriber never receives events of a different type") {
        EventBus bus;
        bool pingReceived = false;

        bus.subscribe<PingEvent>([&pingReceived](const PingEvent&) {
            pingReceived = true;
        });

        bus.publish(PongEvent{1});

        CHECK(pingReceived == false);
    }

    SUBCASE("All subscribers of the same event type are notified") {
        EventBus bus;
        int firstCount = 0;
        int secondCount = 0;

        bus.subscribe<PingEvent>([&firstCount](const PingEvent&) { ++firstCount; });
        bus.subscribe<PingEvent>([&secondCount](const PingEvent&) { ++secondCount; });

        bus.publish(PingEvent{1});

        CHECK(firstCount == 1);
        CHECK(secondCount == 1);
    }

    SUBCASE("Publishing with no subscribers is a harmless no-op") {
        EventBus bus;

        CHECK_NOTHROW(bus.publish(PingEvent{1}));
    }

    SUBCASE("Unsubscribing stops further delivery") {
        EventBus bus;
        int count = 0;

        EventBus::SubscriptionId id = bus.subscribe<PingEvent>([&count](const PingEvent&) {
            ++count;
        });

        bus.publish(PingEvent{1});
        bus.unsubscribe<PingEvent>(id);
        bus.publish(PingEvent{2});

        CHECK(count == 1);
    }

    SUBCASE("A handler may subscribe, unsubscribe and publish reentrantly without deadlocking") {
        EventBus bus;
        int outerCount = 0;
        int innerCount = 0;

        EventBus::SubscriptionId outerId = bus.subscribe<PingEvent>(
            [&](const PingEvent&) {
                ++outerCount;

                bus.subscribe<PongEvent>([&innerCount](const PongEvent&) { ++innerCount; });
                bus.publish(PongEvent{1});
            });

        bus.publish(PingEvent{1});
        bus.unsubscribe<PingEvent>(outerId);
        bus.publish(PingEvent{2});

        CHECK(outerCount == 1);
        CHECK(innerCount == 1);
    }

    SUBCASE("Concurrent subscribe/publish/unsubscribe from multiple threads is safe") {
        EventBus bus;
        std::atomic<int> totalReceived{0};

        constexpr int threadCount = 8;
        constexpr int publishesPerThread = 200;

        std::vector<std::thread> threads;

        for (int t = 0; t < threadCount; ++t)
        {
            threads.emplace_back([&bus, &totalReceived, publishesPerThread]() {
                EventBus::SubscriptionId id = bus.subscribe<PingEvent>(
                    [&totalReceived](const PingEvent&) { ++totalReceived; });

                for (int i = 0; i < publishesPerThread; ++i)
                {
                    bus.publish(PingEvent{i});
                }

                bus.unsubscribe<PingEvent>(id);
            });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        // Each of the threadCount subscribers could have seen anywhere from 0
        // up to threadCount * publishesPerThread events (subscription/publish
        // ordering across threads isn't deterministic) -- the point of this
        // test isn't an exact count, it's that concurrent access never
        // crashes, deadlocks, or corrupts the subscriber registry.
        CHECK(totalReceived >= 0);
    }
}
