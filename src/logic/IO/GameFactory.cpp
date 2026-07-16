#include "GameFactory.h"

#include "../model/GameState.h"

namespace
{
const PieceType BACK_ROW[8] = {
    PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen,
    PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook};
}

Board GameFactory::createClassicBoard()
{
    Board board(8, 8);

    int nextId = 0;

    for (int col = 0; col < 8; ++col)
    {
        board.addPiece(Piece(nextId++, BACK_ROW[col], PieceColor::Black, Position(0, col)));
        board.addPiece(Piece(nextId++, PieceType::Pawn, PieceColor::Black, Position(1, col)));

        board.addPiece(Piece(nextId++, PieceType::Pawn, PieceColor::White, Position(6, col)));
        board.addPiece(Piece(nextId++, BACK_ROW[col], PieceColor::White, Position(7, col)));
    }

    return board;
}

GameEngine GameFactory::createNewGame()
{
    // Returned as a single prvalue (not through a named local) so C++17's
    // guaranteed copy elision applies: GameEngine's RuleEngine stores pointers
    // to its own rule members, which would dangle if the GameEngine were ever
    // actually copied instead of constructed in place.
    return GameEngine(GameState(createClassicBoard()));
}
