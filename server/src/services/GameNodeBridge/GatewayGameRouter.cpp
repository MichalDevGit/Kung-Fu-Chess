#include "GatewayGameRouter.h"

#include <nlohmann/json.hpp>

#include "GameNodeRequestPublisher.h"
#include "../SessionIndex/ISessionIndexStore.h"
#include "../Sharding/IGameShardRoutingStore.h"
#include "common/Config/GameNodeConfig.h"
#include "common/Logging/Logger.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"

GatewayGameRouter::GatewayGameRouter(
    GameNodeRequestPublisher& publisher, ISessionIndexStore& sessionIndexStore, IGameShardRoutingStore& shardRoutingStore)
    : publisher(publisher), sessionIndexStore(sessionIndexStore), shardRoutingStore(shardRoutingStore)
{
}

std::optional<std::string> GatewayGameRouter::shardChannelForConnection(const std::string& connectionId) const
{
    const std::optional<std::string> sessionId = sessionIndexStore.findSessionIdByConnection(connectionId);
    if (!sessionId.has_value())
        return std::nullopt;

    const std::optional<std::string> shardId = shardRoutingStore.findShardForSession(*sessionId);
    if (!shardId.has_value())
        return std::nullopt;

    return GameNodeConfig::shardRequestsChannel(*shardId);
}

std::string GatewayGameRouter::handle(const std::string& connectionId, const std::string& rawJson) const
{
    std::string type;
    try
    {
        type = nlohmann::json::parse(rawJson).value("type", std::string());
    }
    catch (const nlohmann::json::exception&)
    {
        // Falls through to the unknown-type error below, same as a
        // recognized-but-malformed request would.
    }

    if (type == protocol::MessageType::FindGame)
    {
        publisher.forward(GameNodeConfig::MATCHMAKING_REQUESTS_CHANNEL, connectionId, rawJson);
        return "{}";
    }

    if (type == protocol::MessageType::Move || type == protocol::MessageType::Jump)
    {
        const std::optional<std::string> channel = shardChannelForConnection(connectionId);
        if (!channel.has_value())
        {
            // A move/jump for a connection with no routed shard -- either a
            // stale/forged request, or a genuine race between a shard crash
            // and this arriving (see ARCHITECTURE.md's Known gaps). Dropping
            // it is safe either way: the client only ever reacts to the next
            // game_view push, never to this synchronous reply.
            common::Logger::warn("GatewayGameRouter: no shard routed for connection " + connectionId + ", dropping " + type);
            return "{}";
        }

        publisher.forward(*channel, connectionId, rawJson);
        return "{}";
    }

    return protocol::ErrorResult{"unknown_type"}.toJson();
}

void GatewayGameRouter::notifyConnectionClosed(const std::string& connectionId) const
{
    publisher.notifyConnectionClosed(GameNodeConfig::MATCHMAKING_REQUESTS_CHANNEL, connectionId);

    const std::optional<std::string> channel = shardChannelForConnection(connectionId);
    if (channel.has_value())
        publisher.notifyConnectionClosed(*channel, connectionId);
}
