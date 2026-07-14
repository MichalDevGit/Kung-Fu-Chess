#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include "../Controller/Controller.h"
#include "../Controller/BoardMapper.h"

class CommandProcessor
{
private:
    Controller& controller;
    BoardMapper boardMapper;

public:
    explicit CommandProcessor(Controller& controller);

    void run();
};

#endif