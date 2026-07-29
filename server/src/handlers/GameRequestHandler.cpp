#include "handlers/GameRequestHandler.h"

#include <nlohmann/json.hpp>

#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "services/GameSession/GameSessionManager.h"

GameRequestHandler::GameRequestHandler(GameSessionManager& sessionManager)
    : sessionManager(sessionManager)
{
}

std::string GameRequestHandler::handle(const std::string& connectionId, const std::string& rawJson) const
{
    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(rawJson);
        const std::string type = protocol::readType(parsed);

        if (type == protocol::MessageType::Move)
        {
            GameSession* session = sessionManager.findSessionByConnection(connectionId);
            if (session == nullptr)
                return protocol::ErrorResult{"no_active_game"}.toJson();

            const protocol::MoveRequest request = protocol::MoveRequest::fromJson(parsed);
            const GameSession::CommandOutcome outcome = session->requestMove(
                connectionId,
                Position(request.fromRow, request.fromCol),
                Position(request.toRow, request.toCol));

            if (!outcome.accepted)
                return protocol::ErrorResult{outcome.reason}.toJson();

            return protocol::GameViewMessage{session->getGameView()}.toJson();
        }

        if (type == protocol::MessageType::Jump)
        {
            GameSession* session = sessionManager.findSessionByConnection(connectionId);
            if (session == nullptr)
                return protocol::ErrorResult{"no_active_game"}.toJson();

            const protocol::JumpRequest request = protocol::JumpRequest::fromJson(parsed);
            const GameSession::CommandOutcome outcome =
                session->requestJump(connectionId, Position(request.row, request.col));

            if (!outcome.accepted)
                return protocol::ErrorResult{outcome.reason}.toJson();

            return protocol::GameViewMessage{session->getGameView()}.toJson();
        }

        return protocol::ErrorResult{"unknown_type"}.toJson();
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::ErrorResult{"malformed_request"}.toJson();
    }
}
