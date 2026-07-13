#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include "model/GameState.h"

class CommandProcessor
{
private:
    GameState& game;

public:
    CommandProcessor(GameState& game);

    void run();
};

#endif