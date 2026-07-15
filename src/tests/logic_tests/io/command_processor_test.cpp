
#include "tests/doctest.h"
#include "src/logic/IO/CommandProcessor.h"

#include <iostream>
#include <sstream>

TEST_CASE("Testing CommandProcessor (Integration Simulation)") {

    std::stringstream input;
    input << "Board:\n" << "wK .\n" << "Commands:\n" << "print board";
    
    std::streambuf* oldCin = std::cin.rdbuf(input.rdbuf());
    
    CommandProcessor processor;

    processor.run();
    
    std::cin.rdbuf(oldCin);
    }