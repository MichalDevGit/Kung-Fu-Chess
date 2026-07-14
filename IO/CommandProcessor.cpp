#include "CommandProcessor.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace
{
std::string trim(const std::string& text)
{
    size_t first =
        text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
    {
        return "";
    }

    size_t last =
        text.find_last_not_of(" \t\r\n");

    return text.substr(
        first,
        last - first + 1);
}
}

void CommandProcessor::run()
{
    try
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
    catch (const std::exception& exception)
    {
        std::cout << exception.what();
    }
}

std::string CommandProcessor::readBoardText() const
{
    std::ostringstream boardText;

    std::string line;

    while (std::getline(std::cin, line))
    {
        std::string trimmed =
            trim(line);

        if (trimmed == "Commands:")
        {
            break;
        }

        if (trimmed == "Board:")
        {
            continue;
        }

        boardText << trimmed << '\n';
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
        else if (command == "jump")
        {
            int x;
            int y;
        
            input >> x >> y;
        
            controller.jump(
                mapper.pixelToCell(x, y));
        }
    }
}