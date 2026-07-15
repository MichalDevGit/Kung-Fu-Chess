#include "..\..\doctest.h"

#include "..\..\..\model\Piece.h"

TEST_CASE("Testing Piece class functionality") {

    SUBCASE("Default constructor (Empty piece)") {
        Piece p;
        CHECK(p.getId() == -1);
        CHECK(p.isEmpty() == true);
        CHECK(p.toString() == ".");
    }

    SUBCASE("Parameterized constructor and getters") {
        Position pos(2, 3);
        Piece p(10, PieceType::Knight, PieceColor::White, pos);

        CHECK(p.getId() == 10);
        CHECK(p.getType() == PieceType::Knight);
        CHECK(p.getColor() == PieceColor::White);
        CHECK(p.getPosition() == pos);
        CHECK(p.isWhite() == true);
        CHECK(p.isBlack() == false);
        CHECK(p.getState() == PieceState::Idle);
    }

    SUBCASE("Setters and State changes") {
        Piece p;
        p.setState(PieceState::Moved);
        CHECK(p.getState() == PieceState::Moved);

        p.setPosition(Position(5, 5));
        CHECK(p.getPosition() == Position(5, 5));
    }

    SUBCASE("toString representation") {
        Piece whitePawn(1, PieceType::Pawn, PieceColor::White, Position(0,0));
        Piece blackKing(2, PieceType::King, PieceColor::Black, Position(0,0));
        Piece empty;

        CHECK(whitePawn.toString() == "wP");
        CHECK(blackKing.toString() == "bK");
        CHECK(empty.toString() == ".");
    }

    SUBCASE("Equality operator") {
        Position pos(0, 0);
        Piece p1(1, PieceType::Pawn, PieceColor::White, pos);
        Piece p2(1, PieceType::Rook, PieceColor::Black, pos); // אותה ID למרות סוג שונה
        Piece p3(5, PieceType::Pawn, PieceColor::White, pos);

        CHECK(p1 == p2); // אמור להצליח כי ה-ID זהה
        CHECK(p1 != p3); // אמור להצליח כי ה-ID שונה
    }
}