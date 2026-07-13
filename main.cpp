#include <iostream>

#include "IO/BoardParser.h"
#include "IO/BoardPrinter.h"

#include "Model/GameState.h"

#include "Engine/GameEngine.h"

int main()
{
    std::string boardText =
        "bR . . . . . . . "
        ". . . . . . . . "
        ". . . . . . . . "
        ". . . . . . . . "
        ". . . . . . . . "
        ". . . . . . . . "
        ". . . . . . . . "
        "wR . . . . . . .";

    Board board = BoardParser::parse(boardText);

    GameState gameState(board);

    GameEngine engine(gameState);

    std::cout << "Before move:\n";
    std::cout << BoardPrinter::print(engine.getGameState().getBoard()) << std::endl;

    MoveValidation result =
        engine.move(
            Position(7, 0),
            Position(5, 0));

    std::cout << "\nMove valid: "
              << (result.isValid ? "true" : "false")
              << std::endl;

    std::cout << static_cast<int>(result.reason) << std::endl;

    std::cout << "\nAfter move:\n";
    std::cout << BoardPrinter::print(engine.getGameState().getBoard()) << std::endl;

    return 0;
}