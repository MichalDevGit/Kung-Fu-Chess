#ifndef BOARDPARSER_H
#define BOARDPARSER_H

#include <string>

#include "../model/Board.h"

class BoardParser
{
public:
    static Board parse(const std::string& boardText);
};

#endif