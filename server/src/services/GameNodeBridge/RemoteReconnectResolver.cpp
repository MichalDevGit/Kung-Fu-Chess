#include "RemoteReconnectResolver.h"

#include <nlohmann/json.hpp>

#include "GameNodePushRouter.h"
#include "GameNodeRequestPublisher.h"
#include "../Sharding/IGameShardRoutingStore.h"
#include "common/Config/GameNodeConfig.h"

RemoteReconnectResolver::RemoteReconnectResolver(
    GameNodePushRouter& pushRouter, GameNodeRequestPublisher& requestPublisher, IGameShardRoutingStore& shardRoutingStore)
    : pushRouter(pushRouter), requestPublisher(requestPublisher), shardRoutingStore(shardRoutingStore)
{
}

std::optional<protocol::MatchFoundResult> RemoteReconnectResolver::checkAndRebind(
    int userId, const std::string& newConnectionId)
{
    const std::optional<std::string> shardId = shardRoutingStore.findShardForUser(userId);
    if (!shardId.has_value())
        return std::nullopt;

    const std::string channel = GameNodeConfig::shardRequestsChannel(*shardId);

    const std::optional<std::string> replyJson = pushRouter.waitForReconnectCheckResult(
        newConnectionId,
        GameNodeConfig::RECONNECT_CHECK_TIMEOUT_MILLIS,
        [&] { requestPublisher.requestReconnectCheck(channel, newConnectionId, userId); });

    if (!replyJson.has_value() || replyJson->empty())
        return std::nullopt;

    return protocol::MatchFoundResult::fromJson(nlohmann::json::parse(*replyJson));
}
