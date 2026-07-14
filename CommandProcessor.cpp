#include "CommandProcessor.h"

#include <iostream>

#include "IO/BoardPrinter.h"

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
            std::cout << BoardPrinter::print(game.getBoard()) << std::endl;
        }
    }
}