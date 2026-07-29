#ifndef GAME_NODE_PUSH_ROUTER_H
#define GAME_NODE_PUSH_ROUTER_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace sw
{
namespace redis
{
    class Redis;
}
}

// Gateway-side end of GameNodeConfig::PUSHES_CHANNEL (MIGRATION_PLAN.md
// Phase 3). Runs its own background thread consuming Redis pub/sub messages
// published by gamenode/'s GameNodePushPublisher, and does one of two things
// with each one:
//  - kind == "push": hands (connectionId, json) to the default handler --
//    server/src/main.cpp wires this straight to WebSocketServer::sendTo, so
//    every async server-driven message (game_view/match_found/no_match/
//    game_over/opponent_disconnected/opponent_reconnected, and what used to
//    be find_game/move/jump's synchronous reply -- see RemoteGameNodeHandler)
//    reaches the right client exactly as it did in the monolith.
//  - kind == "reconnect_check_result": fulfills whichever
//    waitForReconnectCheckResult() call is currently pending for that
//    connectionId (RemoteReconnectResolver), instead of routing it to the
//    default handler -- this is the one place Phase 3 still needs something
//    resembling a synchronous request/reply, since AuthRequestHandler's
//    completeLogin must know the answer before it can build login_result.
class GameNodePushRouter
{
public:
    using PushHandler = std::function<void(const std::string& connectionId, const std::string& json)>;

    GameNodePushRouter(sw::redis::Redis& redis, PushHandler defaultHandler);

    // Subscribes and starts the background consume loop. Never returns
    // control of that thread -- same "detached, runs forever" shape as
    // server/src/main.cpp's tick thread; this process has nothing useful to
    // do once it stops listening anyway. There is deliberately no stop(): the
    // Redis& passed to the constructor, and this object itself, must outlive
    // that thread -- in practice, both are process-lifetime singletons built
    // once in main() and never destroyed. Constructing/destroying one of
    // these mid-process (e.g. once per test case) is a real use-after-free,
    // not just bad style -- see server/tests/game_node_push_router_test.cpp's
    // file comment for how its own tests learned this the hard way.
    void start();

    // Registers a waiter for connectionId, invokes afterRegistered() (meant
    // to publish the actual "reconnect_check" request -- see
    // RemoteReconnectResolver), then blocks until either a matching
    // "reconnect_check_result" push arrives or timeoutMillis elapses.
    // Returns nullopt on timeout. afterRegistered runs after the waiter is
    // registered specifically so a pathologically fast reply can never race
    // ahead of this call being ready to receive it.
    std::optional<std::string> waitForReconnectCheckResult(
        const std::string& connectionId, long long timeoutMillis, const std::function<void()>& afterRegistered);

private:
    struct Waiter
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;
        std::string payload;
    };

    sw::redis::Redis& redis;
    PushHandler defaultHandler;

    std::mutex waitersMutex;
    std::unordered_map<std::string, std::shared_ptr<Waiter>> waiters;

    void onMessage(const std::string& channel, const std::string& payload);
};

#endif
