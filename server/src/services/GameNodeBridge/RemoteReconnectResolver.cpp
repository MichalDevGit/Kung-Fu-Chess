#include "RemoteReconnectResolver.h"

#include <nlohmann/json.hpp>

#include "GameNodePushRouter.h"
#include "GameNodeRequestPublisher.h"
#include "common/Config/GameNodeConfig.h"

RemoteReconnectResolver::RemoteReconnectResolver(
    GameNodePushRouter& pushRouter, GameNodeRequestPublisher& requestPublisher)
    : pushRouter(pushRouter), requestPublisher(requestPublisher)
{
}

std::optional<protocol::MatchFoundResult> RemoteReconnectResolver::checkAndRebind(
    int userId, const std::string& newConnectionId)
{
    const std::optional<std::string> replyJson = pushRouter.waitForReconnectCheckResult(
        newConnectionId,
        GameNodeConfig::RECONNECT_CHECK_TIMEOUT_MILLIS,
        [&] { requestPublisher.requestReconnectCheck(newConnectionId, userId); });

    if (!replyJson.has_value() || replyJson->empty())
        return std::nullopt;

    return protocol::MatchFoundResult::fromJson(nlohmann::json::parse(*replyJson));
}
