#include "BoardParser.h"

#include <sstream>

namespace
{
int nextPieceId = 0;

PieceType parsePieceType(char typeChar)
{
    switch (typeChar)
    {
        case 'K': return PieceType::King;
        case 'Q': return PieceType::Queen;
        case 'R': return PieceType::Rook;
        case 'B': return PieceType::Bishop;
        case 'N': return PieceType::Knight;
        case 'P': return PieceType::Pawn;
        default:  return PieceType::Empty;
    }
}

PieceColor parsePieceColor(char colorChar)
{
    switch (colorChar)
    {
        case 'w': return PieceColor::White;
        case 'b': return PieceColor::Black;
        default:  return PieceColor::None;
    }
}

Piece parsePieceFromToken(const std::string& token,
                          const Position& position)
{
    PieceColor color = PieceColor::None;
    PieceType type = PieceType::Empty;

    if (token != "." && token.size() >= 2)
    {
        color = parsePieceColor(token[0]);
        type = parsePieceType(token[1]);
    }

    return Piece(nextPieceId++, type, color, position);
}
}

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
            board.addPiece(
                parsePieceFromToken(token, Position(row, col)));
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
