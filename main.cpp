#include <iostream>
#include "Board.h"
#include "GameState.h"
#include "CommandProcessor.h"

int main() {
   
    Board board;
    GameState gameState(board);
    CommandProcessor commandProcessor(gameState);
    commandProcessor.run();

    return 0;
}
