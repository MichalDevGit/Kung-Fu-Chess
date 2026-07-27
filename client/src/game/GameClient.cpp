#include "GameClient.h"

#include <iostream>
#include <nlohmann/json.hpp>

#include "protocol/Message.h"
#include "protocol/MessageType.h"

GameClient::GameClient(WebSocketClient& client, const GameView& initialView)
    : client(client),
      latestView(initialView),
      hasSelection(false),
      selectedPosition(0, 0),
      gameOver(false)
{
    client.setOnMessage([this](const std::string& json)
        {
            onMessage(json);
        });
}

void GameClient::handlePixelClick(const PixelPosition& pixelPosition)
{
    PositionView clicked = boardMapper.pixelToCell(pixelPosition);

    protocol::MoveRequest pendingMove;
    bool shouldSendMove = false;

    {
        std::lock_guard<std::mutex> lock(mutex);

        PieceView clickedPiece = latestView.getBoard().getPiece(clicked.getRow(), clicked.getCol());

        if (!hasSelection)
        {
            if (!clickedPiece.isEmpty())
            {
                hasSelection = true;
                selectedPosition = clicked;
            }
        }
        else
        {
            PieceView selectedPiece =
                latestView.getBoard().getPiece(selectedPosition.getRow(), selectedPosition.getCol());

            if (!clickedPiece.isEmpty() && !selectedPiece.isEmpty() &&
                clickedPiece.getColor() == selectedPiece.getColor())
            {
                selectedPosition = clicked;
            }
            else
            {
                pendingMove = protocol::MoveRequest{
                    selectedPosition.getRow(), selectedPosition.getCol(),
                    clicked.getRow(), clicked.getCol()};
                shouldSendMove = true;
                hasSelection = false;
            }
        }
    }

    // Sent after releasing the lock -- network I/O has no business holding
    // up onMessage() (a different thread) from updating latestView.
    if (shouldSendMove)
    {
        client.send(pendingMove.toJson());
    }
}

void GameClient::handlePixelJump(const PixelPosition& pixelPosition)
{
    PositionView position = boardMapper.pixelToCell(pixelPosition);

    protocol::JumpRequest request{position.getRow(), position.getCol()};
    client.send(request.toJson());
}

GameView GameClient::getGameView() const
{
    std::lock_guard<std::mutex> lock(mutex);

    // The server-reported GameView never carries selection (Controller::move
    // doesn't track it -- see GameClient.h), so this class's own
    // hasSelection/selectedPosition are substituted in here, keeping the
    // selection-highlight rendering feature working under the network split.
    return GameView(
        latestView.getBoard(),
        latestView.getMotion(),
        latestView.getJump(),
        latestView.getRests(),
        hasSelection,
        hasSelection ? selectedPosition : PositionView(),
        latestView.getCurrentTime());
}

bool GameClient::isGameOver() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return gameOver;
}

void GameClient::onMessage(const std::string& json)
{
    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(json);
        const std::string type = protocol::readType(parsed);

        if (type == protocol::MessageType::GameView)
        {
            protocol::GameViewMessage message = protocol::GameViewMessage::fromJson(parsed);

            std::lock_guard<std::mutex> lock(mutex);
            latestView = message.view;
        }
        else if (type == protocol::MessageType::GameOver)
        {
            protocol::GameOverMessage message = protocol::GameOverMessage::fromJson(parsed);

            if (!message.reason.empty())
                std::cout << "\nGame over: " << message.reason << "\n";

            std::lock_guard<std::mutex> lock(mutex);
            gameOver = true;
        }
        else if (type == protocol::MessageType::OpponentDisconnected)
        {
            // Console-only status line -- no rendering work for this yet,
            // the board keeps updating normally (the server's tick loop
            // doesn't pause on a drop, see GameSession::tick).
            std::cout << "\nOpponent disconnected -- waiting for them to reconnect...\n";
        }
        else if (type == protocol::MessageType::OpponentReconnected)
        {
            std::cout << "\nOpponent reconnected.\n";
        }

        // GameStarted/Error: nothing this class tracks yet; parsing them
        // above already validates the envelope, and any other unrecognized
        // type is silently ignored rather than treated as fatal -- a render
        // loop shouldn't crash over one bad frame.
    }
    catch (const nlohmann::json::exception&)
    {
        // Malformed message from the server -- ignore rather than crash the
        // render loop over a single bad frame (same never-throw-out
        // philosophy as AuthRequestHandler/GameRequestHandler on the other
        // end of this same connection).
    }
}
