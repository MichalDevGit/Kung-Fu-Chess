#ifndef BOARDPRINTER_H
#define BOARDPRINTER_H

#include <string>

#include "../Model/Board.h"

class BoardPrinter
{
public:
    static std::string print(const Board& board);
};

#endif