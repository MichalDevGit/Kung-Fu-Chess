#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <string>
#include "../Controller/Controller.h"
#include "../Engine/GameEngine.h"
#include "../Controller/BoardMapper.h"
#include "../model/GameState.h"
#include "BoardParser.h"

class CommandProcessor
{
public:
    void run();

private:
    std::string readBoardText() const;

    void executeCommands(
        class Controller& controller) const;
};

#endif