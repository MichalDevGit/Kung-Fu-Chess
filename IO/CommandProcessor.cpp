#include "CommandProcessor.h"

#include <iostream>
#include <sstream>
#include <string>

CommandProcessor::CommandProcessor(Controller& controller)
    : controller(controller)
{
}

void CommandProcessor::run()
{
    std::string line;

    while (std::getline(std::cin, line))
    {
        if (line == "print board")
        {
            controller.printBoard(std::cout);
            std::cout << std::endl;
            continue;
        }

        std::istringstream input(line);

        std::string command;
        input >> command;

        if (command == "wait")
        {
            long long milliseconds;

            if (input >> milliseconds)
            {
                controller.wait(milliseconds);
            }

            continue;
        }

        if (command == "click")
        {
            int x;
            int y;

            if (input >> x >> y)
            {
                Position position =
                    boardMapper.pixelToCell(x, y);

                controller.click(position);
            }

            continue;
        }
    }
}