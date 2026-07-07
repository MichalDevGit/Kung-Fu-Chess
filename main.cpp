#include <iostream>
#include "Board.h"
#include "GameState.h"
#include "CommandProcessor.h"
#include <sstream>

bool isValidToken(const std::string& token) {
    if (token == ".") return true;
    if (token.length() != 2) return false;
    bool validColor = (token[0] == 'w' || token[0] == 'b');
    bool validPiece = (token[1] == 'K' || token[1] == 'Q' || token[1] == 'R' || 
                       token[1] == 'B' || token[1] == 'N' || token[1] == 'P');
    return validColor && validPiece;
}

int main() {

    std::vector<std::vector<std::string>> grid;

    
    std::string line;
    size_t expectedCols = 0;
    
    while (std::getline(std::cin, line) && line != "Commands:") {
        if (line == "Board:") continue;
        if (line.empty()) continue;
        
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> row;
    
    while (ss >> token) {
        if (!isValidToken(token)) {
            std::cout << "ERROR UNKNOWN_TOKEN" << std::endl;
            return 0;
        }
            row.push_back(token);
        }
        
        if (!row.empty()) {
            if (expectedCols == 0) {
                expectedCols = row.size();
            } else if (row.size() != expectedCols) {
                std::cout << "ERROR ROW_WIDTH_MISMATCH" << std::endl;
                return 0;
            }
                grid.push_back(row);
            }
        }
    
    Board board(grid);
    
    GameState game(board);
    
    CommandProcessor processor(game);

    processor.run();

    return 0;
}
