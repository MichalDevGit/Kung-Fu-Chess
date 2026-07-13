#include "BoardParser.h"

#include <sstream>

Board BoardParser::parse(const std::string& boardText)
{
    Board board(8, 8);

    std::istringstream input(boardText);

    std::string token;

    int row = 0;
    int col = 0;

    while (input >> token)
    {
        if (token != ".")
        {
            Piece piece(token);

            piece.setPosition(Position(row, col));

            board.addPiece(piece);
        }

        col++;

        if (col == board.getCols())
        {
            col = 0;
            row++;
        }
    }

    return board;
}