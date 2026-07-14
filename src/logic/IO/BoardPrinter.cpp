#include "BoardPrinter.h"

#include <sstream>

std::string BoardPrinter::print(const Board& board)
{
    std::ostringstream output;

    for (int row = 0; row < board.getRows(); row++)
    {
        for (int col = 0; col < board.getCols(); col++)
        {
            const Piece* piece = board.getPiece(Position(row, col));

            if (piece != nullptr)
            {
                output << piece->toString();
            }
            else
            {
                output << ".";
            }

            if (col + 1 < board.getCols())
            {
                output << " ";
            }
        }

        if (row + 1 < board.getRows())
        {
            output << '\n';
        }
    }

    return output.str();
}