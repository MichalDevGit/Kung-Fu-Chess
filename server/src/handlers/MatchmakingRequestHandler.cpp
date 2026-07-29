#include "handlers/MatchmakingRequestHandler.h"

#include <nlohmann/json.hpp>
#include <optional>

#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "services/Connection/ConnectionRegistry.h"
#include "services/GameSession/GameSessionManager.h"
#include "services/Matchmaking/Matchmaker.h"
#include "common/MonotonicClock.h"

MatchmakingRequestHandler::MatchmakingRequestHandler(
    ConnectionRegistry& connectionRegistry, Matchmaker& matchmaker, GameSessionManager& sessionManager)
    : connectionRegistry(connectionRegistry)
    , matchmaker(matchmaker)
    , sessionManager(sessionManager)
{
}

std::string MatchmakingRequestHandler::handle(const std::string& connectionId, const std::string& rawJson) const
{
    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(rawJson);
        const std::string type = protocol::readType(parsed);

        if (type != protocol::MessageType::FindGame)
            return protocol::ErrorResult{"unknown_type"}.toJson();

        const std::optional<ConnectionRegistry::AuthenticatedUser> user = connectionRegistry.find(connectionId);
        if (!user.has_value())
            return protocol::ErrorResult{"not_authenticated"}.toJson();

        if (sessionManager.findSessionByConnection(connectionId) != nullptr)
            return protocol::ErrorResult{"already_in_game"}.toJson();

        if (matchmaker.isQueued(connectionId))
            return protocol::SearchingResult{}.toJson();

        matchmaker.enqueue(Matchmaker::Entry{connectionId, user->userId, user->username, user->score, nowMillis()});

        return protocol::SearchingResult{}.toJson();
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::ErrorResult{"malformed_request"}.toJson();
    }
}
