
#include "tests/doctest.h"
#include "src/logic/model/Board.h"
#include <stdexcept>

TEST_CASE("Testing Board class functionality") {
    Board board(8, 8);

    SUBCASE("Initialization and dimensions") {
        CHECK(board.getRows() == 8);
        CHECK(board.getCols() == 8);
    }

    SUBCASE("Valid and invalid positions") {
        CHECK(board.isValidPosition(Position(0, 0)) == true);
        CHECK(board.isValidPosition(Position(7, 7)) == true);
        CHECK(board.isValidPosition(Position(8, 0)) == false); // מחוץ ללוח
        CHECK(board.isValidPosition(Position(0, -1)) == false);
    }

    SUBCASE("Adding pieces") {
        Piece p(1, PieceType::Pawn, PieceColor::White, Position(0, 0));
        board.addPiece(p);

        CHECK(board.containsPiece(Position(0, 0)) == true);
        CHECK(board.getPiece(Position(0, 0)) != nullptr);
        
        // בדיקת זריקת חריגה בהוספה למקום תפוס
        CHECK_THROWS_AS(board.addPiece(p), std::invalid_argument);
    }

    SUBCASE("Removing pieces") {
        Position pos(1, 1);
        Piece p(2, PieceType::Rook, PieceColor::Black, pos);
        board.addPiece(p);
        
        board.removePiece(pos);
        CHECK(board.containsPiece(pos) == false);
        CHECK(board.getPiece(pos) == nullptr);
    }

    SUBCASE("Moving pieces") {
        Position from(2, 2);
        Position to(3, 3);
        Piece p(3, PieceType::Knight, PieceColor::White, from);
        
        board.addPiece(p);
        board.movePiece(from, to);

        CHECK(board.containsPiece(from) == false);
        CHECK(board.containsPiece(to) == true);
        CHECK(board.getPiece(to)->getId() == 3);
    }
}