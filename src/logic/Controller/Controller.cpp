#include "Controller.h"

#include <ostream>

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

BoardView Controller::getBoardView() const
{
    return BoardView(gameEngine.getGameState().getBoard());
}

bool Controller::isGameOver() const
{
    return gameEngine.getGameState().isGameOver();
}