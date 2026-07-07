#include "CommandProcessor.h"

#include <iostream>
#include <sstream>

CommandProcessor::CommandProcessor(GameState& g)
    : game(g)
{
}

void CommandProcessor::run()
{
    std::string line;

    while (std::getline(std::cin, line))
    {
        if (line == "print board")
        {
            game.printBoard();
        }
        else if (line.rfind("click", 0) == 0)
        {
            std::stringstream ss(line);

            std::string command;
            int x;
            int y;

            ss >> command >> x >> y;

            game.handleClick(x, y);
        }
        else if (line.rfind("wait", 0) == 0)
        {
            std::stringstream ss(line);

            std::string command;
            int milliseconds;

            ss >> command >> milliseconds;

            game.wait(milliseconds);
        }
    }
}