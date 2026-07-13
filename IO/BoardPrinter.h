#ifndef BOARDPRINTER_H
#define BOARDPRINTER_H

#include <string>

#include "../model/Board.h"

class BoardPrinter
{
public:
    static std::string print(const Board& board);
};

#endif