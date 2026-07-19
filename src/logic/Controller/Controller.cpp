#include "Controller.h"

#include <ostream>

#include "../../common/DTO/BoardView.h"
#include "../../common/DTO/MotionView.h"
#include "../../common/DTO/JumpView.h"
#include "../../common/DTO/RestView.h"
#include "../model/Rest.h"

Controller::Controller(GameEngine& gameEngine)
    : gameEngine(gameEngine),
      hasSelection(false),
      selectedPosition(0, 0)
{
}

void Controller::click(const Position& position)
{
    const Board& board =
        gameEngine.getGameState().getBoard();

    if (!board.isValidPosition(position))
    {
        return;
    }

    const Piece* clickedPiece =
        board.getPiece(position);

    if (!hasSelection)
    {
        if (clickedPiece == nullptr)
        {
            return;
        }

        hasSelection = true;
        selectedPosition = position;
        return;
    }

    const Piece* selectedPiece =
        board.getPiece(selectedPosition);

    if (clickedPiece != nullptr &&
        selectedPiece != nullptr &&
        clickedPiece->getColor() == selectedPiece->getColor())
    {
        selectedPosition = position;
        return;
    }

    gameEngine.requestMove(
        selectedPosition,
        position);

    hasSelection = false;
}

void Controller::handlePixelClick(const PixelPosition& pixelPosition)
{
    click(boardMapper.pixelToCell(pixelPosition));
}

void Controller::handlePixelJump(const PixelPosition& pixelPosition)
{
    jump(boardMapper.pixelToCell(pixelPosition));
}

void Controller::wait(long long milliseconds){
    gameEngine.advanceTime(milliseconds);
}

void Controller::printBoard(std::ostream& out) const{
    out << BoardPrinter::print(
        gameEngine.getGameState().getBoard());
}

void Controller::jump(const Position& position)
{
    gameEngine.requestJump(position);
}

bool Controller::hasSelectedPiece() const
{
    return hasSelection;
}

Position Controller::getSelectedPosition() const
{
    return selectedPosition;
}

GameView Controller::getGameView() const
{
    BoardView boardView(gameEngine.getGameState().getBoard());

    MotionView motionView = gameEngine.hasActiveMotion()
        ? MotionView(gameEngine.getCurrentMotion())
        : MotionView();

    JumpView jumpView = gameEngine.hasActiveJump()
        ? JumpView(gameEngine.getCurrentJump())
        : JumpView();

    std::vector<RestView> rests;

    for (const Rest& rest : gameEngine.getActiveRests())
    {
        const Piece* piece =
            gameEngine.getGameState().getBoard().getPieceById(rest.getPieceId());

        if (piece != nullptr)
        {
            rests.emplace_back(
                rest.getPieceId(),
                PositionView(piece->getPosition()),
                rest.getStartTime(),
                rest.getEndTime(),
                rest.getKind());
        }
    }

    PositionView selectedPositionView = hasSelection
        ? PositionView(selectedPosition)
        : PositionView();

    return GameView(
        boardView,
        motionView,
        jumpView,
        rests,
        hasSelection,
        selectedPositionView,
        gameEngine.getCurrentTime());
}

bool Controller::isGameOver() const
{
    return gameEngine.getGameState().isGameOver();
}