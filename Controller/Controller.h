#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../Engine/GameEngine.h"
#include "../Model/Position.h"

class Controller
{
public:
    explicit Controller(GameEngine& gameEngine);

    void click(const Position& position);

    bool hasSelectedPiece() const;
    Position getSelectedPosition() const;

private:
    GameEngine& gameEngine;

    bool hasSelection;
    Position selectedPosition;
};

#endif