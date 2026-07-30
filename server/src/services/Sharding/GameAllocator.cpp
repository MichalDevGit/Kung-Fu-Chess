#include "GameAllocator.h"

#include <stdexcept>
#include <utility>

#include "IGameShardRoutingStore.h"
#include "IShardLoadStore.h"
#include "../../game/Controller/Controller.h"
#include "../../game/IO/GameFactory.h"
#include "../../../../common/EventBus/EventBus.h"

namespace
{
// The initial GameView every freshly matched session starts from is always
// identical -- GameFactory::createClassicBoard assigns fixed piece ids
// (0, 1, 2, ...) every single call, and a fresh GameEngine/RealTimeArbiter's
// clock always starts at 0 -- so this class can compute it locally instead
// of waiting for whichever Game Server Shard actually ends up hosting the
// session to build and report one back. A throwaway EventBus/GameEngine/
// Controller (never a GameSession) is enough: GameSession is the layer that
// has push side effects (SendToFn, EventBus subscribers), and none of that
// is touched here.
GameView initialGameView()
{
    EventBus eventBus;
    GameEngine engine = GameFactory::createNewGame(eventBus);
    Controller controller(engine);
    return controller.getGameView();
}
}

GameAllocator::GameAllocator(
    IShardLoadStore& loadStore, IGameShardRoutingStore& routingStore, RequestSessionCreationFn requestSessionCreation)
    : loadStore(loadStore), routingStore(routingStore), requestSessionCreation(std::move(requestSessionCreation))
{
}

std::string GameAllocator::pickShard() const
{
    const std::vector<std::string> shards = loadStore.knownShards();
    if (shards.empty())
        throw std::runtime_error("GameAllocator::allocate: no Game Server Shard is registered");

    std::string best = shards.front();
    long long bestLoad = loadStore.loadOf(best);

    for (const std::string& shardId : shards)
    {
        const long long load = loadStore.loadOf(shardId);
        if (load < bestLoad)
        {
            best = shardId;
            bestLoad = load;
        }
    }

    return best;
}

std::string GameAllocator::nextSessionId()
{
    std::lock_guard<std::mutex> lock(mutex);
    return "session-" + std::to_string(nextSessionNumber++);
}

GameAllocator::Allocation GameAllocator::allocate(const Match& match)
{
    const std::string shardId = pickShard();
    const std::string sessionId = nextSessionId();

    GameSession::Player white{match.first.userId, match.first.username, match.first.connectionId};
    GameSession::Player black{match.second.userId, match.second.username, match.second.connectionId};

    requestSessionCreation(sessionId, white, black, shardId);

    routingStore.bindSession(sessionId, {match.first.userId, match.second.userId}, shardId);
    loadStore.incrementLoad(shardId);

    return Allocation{sessionId, initialGameView()};
}
