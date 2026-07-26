#include "handlers/GameRequestHandler.h"

#include <nlohmann/json.hpp>

#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "services/GameSessionManager.h"

GameRequestHandler::GameRequestHandler(GameSessionManager& sessionManager)
    : sessionManager(sessionManager)
{
}

std::string GameRequestHandler::handle(const std::string& rawJson) const
{
    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(rawJson);
        const std::string type = protocol::readType(parsed);

        if (type == protocol::MessageType::JoinGame)
        {
            GameSession& session = sessionManager.getOrCreateDefaultSession();
            return protocol::GameJoinedResult{session.getId()}.toJson();
        }

        if (type == protocol::MessageType::Move)
        {
            const protocol::MoveRequest request = protocol::MoveRequest::fromJson(parsed);
            GameSession& session = sessionManager.getOrCreateDefaultSession();

            session.requestMove(
                Position(request.fromRow, request.fromCol),
                Position(request.toRow, request.toCol));

            return protocol::GameViewMessage{session.getGameView()}.toJson();
        }

        if (type == protocol::MessageType::Jump)
        {
            const protocol::JumpRequest request = protocol::JumpRequest::fromJson(parsed);
            GameSession& session = sessionManager.getOrCreateDefaultSession();

            session.requestJump(Position(request.row, request.col));

            return protocol::GameViewMessage{session.getGameView()}.toJson();
        }

        return protocol::ErrorResult{"unknown_type"}.toJson();
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::ErrorResult{"malformed_request"}.toJson();
    }
}
