#include "CommandProcessor.h"

#include <iostream>
#include <sstream>

#include "../Controller/Controller.h"
#include "../Engine/GameEngine.h"
#include "../IO/BoardMapper.h"
#include "../IO/BoardParser.h"
#include "../Model/GameState.h"

void CommandProcessor::run()
{
    std::string boardText =
        readBoardText();

    Board board =
        BoardParser::parse(boardText);

    GameState gameState(board);

    GameEngine engine(gameState);

    Controller controller(engine);

    executeCommands(controller);
}

std::string CommandProcessor::readBoardText() const
{
    std::ostringstream boardText;

    std::string line;

    while (std::getline(std::cin, line))
    {
        if (line == "Commands:")
        {
            break;
        }

        if (line == "Board:")
        {
            continue;
        }

        boardText << line << '\n';
    }

    return boardText.str();
}

void CommandProcessor::executeCommands(
    Controller& controller) const
{
    BoardMapper mapper;

    std::string line;

    while (std::getline(std::cin, line))
    {
        std::istringstream input(line);

        std::string command;

        input >> command;

        if (command == "click")
        {
            int x;
            int y;

            input >> x >> y;

            controller.click(
                mapper.pixelToCell(x, y));
        }
        else if (command == "wait")
        {
            long long milliseconds;

            input >> milliseconds;

            controller.wait(milliseconds);
        }
        else if (command == "print")
        {
            std::string what;

            input >> what;

            if (what == "board")
            {
                controller.printBoard(std::cout);
                std::cout << '\n';
            }
        }
    }
}