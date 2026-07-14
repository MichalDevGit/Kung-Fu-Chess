#include "BoardParser.h"

#include <sstream>
#include <vector>
#include <stdexcept>

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

bool isValidToken(const std::string& token)
{
    if (token == ".")
    {
        return true;
    }

    if (token.size() != 2)
    {
        return false;
    }

    PieceColor color =
        parsePieceColor(token[0]);

    PieceType type =
        parsePieceType(token[1]);

    return color != PieceColor::None &&
           type != PieceType::Empty;
}

Piece parsePieceFromToken(
    const std::string& token,
    const Position& position)
{
    PieceColor color =
        parsePieceColor(token[0]);

    PieceType type =
        parsePieceType(token[1]);

    return Piece(
        nextPieceId++,
        type,
        color,
        position);
}
}

Board BoardParser::parse(const std::string& boardText)
{
    std::istringstream input(boardText);

    std::vector<std::vector<std::string>> rows;

    std::string line;

    while (std::getline(input, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::istringstream lineStream(line);

        std::vector<std::string> tokens;

        std::string token;

        while (lineStream >> token)
        {
            if (!isValidToken(token))
            {
                throw std::runtime_error(
                    "ERROR UNKNOWN_TOKEN");
            }

            tokens.push_back(token);
        }

        if (!tokens.empty())
        {
            rows.push_back(tokens);
        }
    }

    if (rows.empty())
    {
        return Board(0, 0);
    }

    int cols =
        rows[0].size();

    for (const auto& row : rows)
    {
        if (row.size() != cols)
        {
            throw std::runtime_error(
                "ERROR ROW_WIDTH_MISMATCH");
        }
    }

    Board board(
        rows.size(),
        cols);

    for (int row = 0; row < (int)rows.size(); row++)
    {
        for (int col = 0; col < cols; col++)
        {
            if (rows[row][col] == ".")
            {
                continue;
            }

            board.addPiece(
                parsePieceFromToken(
                    rows[row][col],
                    Position(row, col)));
        }
    }

    return board;
}