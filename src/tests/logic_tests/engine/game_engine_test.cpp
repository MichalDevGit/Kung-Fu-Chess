#include "tests/doctest.h"
#include "src/logic/Engine/GameEngine.h"

TEST_CASE("Testing GameEngine flow") {
    // אתחול לוח ומשחק
    Board board(8, 8);
    GameState state(board);
    GameEngine engine(state);

    // הוספת כלים לבדיקה
    Piece p1(1, PieceType::Pawn, PieceColor::White, Position(6, 4));
    board.addPiece(p1);

    SUBCASE("Requesting a valid move") {
        MoveValidation result = engine.requestMove(Position(6, 4), Position(5, 4));
        CHECK(result.isValid == true);
        CHECK(engine.hasActiveMotion() == true);
    }

    SUBCASE("Advancing time and finishing motion") {
        engine.requestMove(Position(6, 4), Position(5, 4));
        
        // קידום זמן מספיק לסיום המהלך (1000ms לפי MILLIS_PER_SQUARE)
        engine.advanceTime(1500);
        
        CHECK(engine.hasActiveMotion() == false);
        CHECK(engine.hasPieceAt(Position(5, 4)) == true);
        CHECK(engine.hasPieceAt(Position(6, 4)) == false);
    }

    SUBCASE("Game Over on King capture") {
        Piece king(2, PieceType::King, PieceColor::Black, Position(5, 4));
        board.addPiece(king);
        
        engine.requestMove(Position(6, 4), Position(5, 4));
        engine.advanceTime(1500);
        
        CHECK(engine.getGameState().isGameOver() == true);
    }

    SUBCASE("Pawn promotion") {
        // רגלי מגיע לשורה האחרונה
        Piece pawn(1, PieceType::Pawn, PieceColor::White, Position(1, 4));
        board.addPiece(pawn);
        
        engine.requestMove(Position(1, 4), Position(0, 4));
        engine.advanceTime(1500);
        
        // בדיקה שהרגלי הפך למלכה
        CHECK(engine.getGameState().getBoard().getPiece(Position(0, 4))->getType() == PieceType::Queen);
    }
}