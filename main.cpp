#include "Model/Board.h"
#include "Model/GameState.h"
#include "Engine/GameEngine.h"
#include "Controller/Controller.h"

int main()
{
    Board board(8, 8);

    GameState gameState(board);

    GameEngine engine(gameState);

    Controller controller(engine);

    return 0;
}