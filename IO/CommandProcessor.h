#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <string>

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