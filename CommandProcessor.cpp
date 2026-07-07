#include <iostream>
#include <string>
#include <sstream>
#include "GameState.h"

class CommandProcessor {
private:
    GameState& game;

public:
    CommandProcessor(GameState& g) : game(g) {}

    void run() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "print board") {
                game.printBoard();
            } 
            else if (line.find("click") == 0) {
                std::stringstream ss(line);
                std::string cmd;
                int x, y;
                ss >> cmd >> x >> y;
                game.handleClick(x, y);
            }
            else if (line.find("wait") == 0) {
                std::stringstream ss(line);
                std::string cmd;
                int ms;
                ss >> cmd >> ms;
                game.addTime(ms);
            }
        }
    }
};