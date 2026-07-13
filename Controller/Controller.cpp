#include "Controller.h"

Controller::Controller(GameEngine& gameEngine)
    : gameEngine(gameEngine),
      hasSelection(false),
      selectedPosition(0, 0)
{
}

void Controller::click(const Position& position)
{
    if (!hasSelection)
    {

        if (!gameEngine.hasPieceAt(position))
        {
            return;
        }

        hasSelection = true;
        selectedPosition = position;
        return;
    }

    MoveValidation result =
    gameEngine.requestMove(selectedPosition, position);
    
    hasSelection = false;
}

bool Controller::hasSelectedPiece() const
{
    return hasSelection;
}

Position Controller::getSelectedPosition() const
{
    return selectedPosition;
}