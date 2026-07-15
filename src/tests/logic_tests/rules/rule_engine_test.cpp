#include "../../../doctest.h"
#include "../../../src/logic/rules/RuleEngine.h"

TEST_CASE("Testing RuleEngine validation") {
    RuleEngine engine;
    Board board(8, 8);

    // הגדרת כלים לצורך בדיקה
    Piece whitePawn(1, PieceType::Pawn, PieceColor::White, Position(6, 4));
    Piece blackRook(2, PieceType::Rook, PieceColor::Black, Position(0, 0));
    board.addPiece(whitePawn);
    board.addPiece(blackRook);

    SUBCASE("Valid move validation") {
        // רגלי לבן יכול להתקדם מ-(6,4) ל-(5,4)
        MoveValidation result = engine.validateMove(board, Position(6, 4), Position(5, 4));
        CHECK(result.isValid == true);
        CHECK(result.reason == MoveValidationReason::Valid);
    }

    SUBCASE("Invalid move - Friendly piece collision") {
        // הוספת כלי לבן נוסף בדרך של הרגלי
        Piece whiteKnight(3, PieceType::Knight, PieceColor::White, Position(5, 4));
        board.addPiece(whiteKnight);

        MoveValidation result = engine.validateMove(board, Position(6, 4), Position(5, 4));
        CHECK(result.isValid == false);
        CHECK(result.reason == MoveValidationReason::FriendlyPieceAtDestination);
    }

    SUBCASE("Invalid move - Empty source") {
        // ניסיון להזיז כלי ממשבצת ריקה
        MoveValidation result = engine.validateMove(board, Position(3, 3), Position(3, 4));
        CHECK(result.isValid == false);
        CHECK(result.reason == MoveValidationReason::SourceEmpty);
    }

    SUBCASE("Invalid move - Outside board") {
        MoveValidation result = engine.validateMove(board, Position(0, 0), Position(-1, 0));
        CHECK(result.isValid == false);
        CHECK(result.reason == MoveValidationReason::DestinationOutsideBoard);
    }

    SUBCASE("Illegal move - Movement rules mismatch") {
        // ניסיון להזיז רגלי בצורה של פרש (לא חוקי)
        MoveValidation result = engine.validateMove(board, Position(6, 4), Position(4, 3));
        CHECK(result.isValid == false);
        CHECK(result.reason == MoveValidationReason::IllegalMovement);
    }
}