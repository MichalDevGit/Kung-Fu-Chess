
#include "tests/doctest.h"
#include "src/logic/rules/BishopRule.h"
#include "src/logic/rules/KingRule.h"
#include "src/logic/rules/KnightRule.h"
#include "src/logic/rules/PawnRule.h"
#include "src/logic/rules/QueenRule.h"
#include "src/logic/rules/RookRule.h"
#include "src/logic/model/Board.h"

TEST_CASE("Testing Movement Rules") {
    Board board(8, 8);

    SUBCASE("Bishop Movement") {
        BishopRule rule;
        Piece bishop(1, PieceType::Bishop, PieceColor::White, Position(4, 4));
        board.addPiece(bishop);
        
        auto destinations = rule.legalDestinations(board, bishop);
        // בדיקה לדוגמה: אמור למצוא אלכסונים
        CHECK(destinations.count(Position(3, 3)) == 1);
        CHECK(destinations.count(Position(5, 5)) == 1);
    }

    SUBCASE("Knight Movement") {
        KnightRule rule;
        Piece knight(1, PieceType::Knight, PieceColor::White, Position(4, 4));
        board.addPiece(knight);
        
        auto destinations = rule.legalDestinations(board, knight);
        // בדיקת קפיצה תקינה של פרש
        CHECK(destinations.count(Position(2, 3)) == 1);
        CHECK(destinations.count(Position(6, 5)) == 1);
    }

    SUBCASE("Pawn Movement") {
        PawnRule rule;
        // לבן מתחיל בשורה 6, נע למעלה (row - 1)
        Piece pawn(1, PieceType::Pawn, PieceColor::White, Position(6, 4));
        board.addPiece(pawn);
        
        auto destinations = rule.legalDestinations(board, pawn);
        CHECK(destinations.count(Position(5, 4)) == 1);
        CHECK(destinations.count(Position(4, 4)) == 1); // צעד כפול מההתחלה
    }

    SUBCASE("King Movement (Safety Check)") {
        KingRule rule;
        Piece king(1, PieceType::King, PieceColor::White, Position(4, 4));
        board.addPiece(king);
        
        auto destinations = rule.legalDestinations(board, king);
        CHECK(destinations.size() == 8); // כל הכיוונים מסביב
    }
}