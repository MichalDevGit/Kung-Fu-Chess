#include "GameState.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

void GameState::handleClick(int x, int y) {
    int row = y / 100;
    int col = x / 100;

    if (!board.isValidPosition(row, col)) return;

    std::string clickedPiece = board.getPiece(row, col);

    if (selectedRow == -1) {
        if (clickedPiece != ".") {
            selectedRow = row;
            selectedCol = col;
        }
    } else {

        if (clickedPiece != "." && clickedPiece[0] == board.getPiece(selectedRow, selectedCol)[0]) {
            selectedRow = row;
            selectedCol = col;
        } else {
            board.setPiece(row, col, board.getPiece(selectedRow, selectedCol));
            board.setPiece(selectedRow, selectedCol, ".");
            selectedRow = -1;
            selectedCol = -1;
        }
    }
}

void GameState::addTime(int ms) {
    gameClock += ms;
}

bool GameState::isValidToken(const std::string& token) {
    if (token == ".") return true;
    if (token.length() != 2) return false;

    char color = token[0];
    char piece = token[1];

    bool validColor = (color == 'w' || color == 'b');
    bool validPiece = (piece == 'K' || piece == 'Q' || piece == 'R' || 
                       piece == 'B' || piece == 'N' || piece == 'P');

    return validColor && validPiece;
}

void GameState::printBoard() {

    std::string line;
    std::vector<std::vector<std::string>> board;
    size_t expectedCols = 0;

    while (std::getline(std::cin, line) && line != "Commands:") {
        if (line == "Board:") continue;
        
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
            board.push_back(row);
        }
    }

    while (std::getline(std::cin, line)) {
        if (line == "print board") {
            for (size_t i = 0; i < board.size(); ++i) {
                for (size_t j = 0; j < board[i].size(); ++j) {
                    std::cout << board[i][j] << (j == board[i].size() - 1 ? "" : " ");
                }
                std::cout << std::endl;
            }
        }
    }
}