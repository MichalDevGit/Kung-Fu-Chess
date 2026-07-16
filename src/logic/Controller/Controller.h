#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <iosfwd>

#include "../Engine/GameEngine.h"
#include "../model/Position.h"
#include "../IO/BoardPrinter.h"
#include "BoardMapper.h"
#include "../../common/PixelPosition.h"
#include "../../common/DTO/BoardView.h"

class Controller
{
public:
    explicit Controller(GameEngine& gameEngine);

    void click(const Position& position);
    void handlePixelClick(const PixelPosition& pixelPosition);
    void wait(long long milliseconds);
    void printBoard(std::ostream& out) const;
    void jump(const Position& position);

    bool hasSelectedPiece() const;
    Position getSelectedPosition() const;

    BoardView getBoardView() const;
    bool isGameOver() const;

private:
    GameEngine& gameEngine;
    BoardMapper boardMapper;

    bool hasSelection;
    Position selectedPosition;
};

#endif