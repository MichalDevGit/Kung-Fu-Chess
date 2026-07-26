#ifndef GAMEFACTORY_H
#define GAMEFACTORY_H

#include "../Engine/GameEngine.h"
#include "../model/Board.h"

class GameFactory
{
public:
    static GameEngine createNewGame(EventBus& eventBus);

private:
    static Board createClassicBoard();
};

#endif
