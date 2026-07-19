#ifndef BOARD_H
#define BOARD_H

#include <vector>

#include "Piece.h"
#include "Position.h"

class Board
{
private:
    int rows;
    int cols;

    std::vector<Piece> pieces;

public:
    Board(int rows, int cols);

    int getRows() const;
    int getCols() const;

    bool isValidPosition(const Position& position) const;

    Piece* getPiece(const Position& position);
    const Piece* getPiece(const Position& position) const;

    Piece* getPieceById(int id);
    const Piece* getPieceById(int id) const;

    void addPiece(const Piece& piece);

    void removePiece(const Position& position);

    void movePiece(const Position& from,
                   const Position& to);

    const std::vector<Piece>& getPieces() const;
    bool containsPiece(const Position& position) const;
};

#endif