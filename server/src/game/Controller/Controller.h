#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <iosfwd>

#include "../Engine/GameEngine.h"
#include "../model/Position.h"
#include "../IO/BoardPrinter.h"
#include "../../../../common/DTO/GameView.h"

class Controller
{
public:
    explicit Controller(GameEngine& gameEngine);

    void click(const Position& position);
    void wait(long long milliseconds);
    void printBoard(std::ostream& out) const;
    void jump(const Position& position);

    // Direct from/to move command -- bypasses click()'s select-then-move
    // state machine entirely. click() exists for a physical-mouse UI
    // affordance (first click selects, second click acts); a networked
    // MoveRequest already names both squares explicitly, so routing it
    // through click()'s selection state would risk misfiring against
    // whatever this Controller's selection happened to be at the time.
    void move(const Position& from, const Position& to);

    bool hasSelectedPiece() const;
    Position getSelectedPosition() const;

    GameView getGameView() const;
    bool isGameOver() const;

private:
    GameEngine& gameEngine;

    bool hasSelection;
    Position selectedPosition;
};

#endif