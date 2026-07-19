#include "tests/doctest.h"
#include "src/logic/Engine/GameEngine.h"

TEST_CASE("Testing GameEngine flow") {
    // אתחול לוח ומשחק
    Board board(8, 8);

    // הוספת כלים לבדיקה (לפני בניית GameState/GameEngine, שכן שניהם מעתיקים
    // את הלוח בזמן הבנייה - כלים שנוספים אחר כך לא ישפיעו על ה-engine)
    Piece p1(1, PieceType::Pawn, PieceColor::White, Position(6, 4));
    board.addPiece(p1);

    SUBCASE("Requesting a valid move") {
        GameState state(board);
        GameEngine engine(state);

        MoveValidation result = engine.requestMove(Position(6, 4), Position(5, 4));
        CHECK(result.isValid == true);
        CHECK(engine.hasActiveMotion() == true);
    }

    SUBCASE("Advancing time and finishing motion") {
        GameState state(board);
        GameEngine engine(state);

        engine.requestMove(Position(6, 4), Position(5, 4));

        // קידום זמן מספיק לסיום המהלך (1000ms לפי MILLIS_PER_SQUARE)
        engine.advanceTime(1500);

        CHECK(engine.hasActiveMotion() == false);
        CHECK(engine.hasPieceAt(Position(5, 4)) == true);
        CHECK(engine.hasPieceAt(Position(6, 4)) == false);
    }

    SUBCASE("Game Over on King capture") {
        // Diagonally in front of the pawn - pawns only capture diagonally,
        // a straight-ahead square would make the move illegal.
        Piece king(2, PieceType::King, PieceColor::Black, Position(5, 3));
        board.addPiece(king);

        GameState state(board);
        GameEngine engine(state);

        engine.requestMove(Position(6, 4), Position(5, 3));
        engine.advanceTime(1500);

        CHECK(engine.getGameState().isGameOver() == true);
    }

    SUBCASE("Pawn promotion") {
        // רגלי נוסף שמגיע לשורה האחרונה (מזהה נפרד מ-p1 כדי לא להתנגש איתו)
        Piece pawn(3, PieceType::Pawn, PieceColor::White, Position(1, 4));
        board.addPiece(pawn);

        GameState state(board);
        GameEngine engine(state);

        engine.requestMove(Position(1, 4), Position(0, 4));
        engine.advanceTime(1500);

        // בדיקה שהרגלי הפך למלכה
        CHECK(engine.getGameState().getBoard().getPiece(Position(0, 4))->getType() == PieceType::Queen);
    }

    SUBCASE("Piece rests after completing a move") {
        GameState state(board);
        GameEngine engine(state);

        engine.requestMove(Position(6, 4), Position(5, 4));
        engine.advanceTime(1500);

        CHECK(engine.isPieceResting(1) == true);

        MoveValidation second = engine.requestMove(Position(5, 4), Position(4, 4));
        CHECK(second.isValid == false);
        CHECK(second.reason == MoveValidationReason::PieceResting);

        // matches GameEngine::REST_DURATION_MILLIS (private, referenced as a
        // literal here the same way MILLIS_PER_SQUARE is above)
        engine.advanceTime(2000);
        CHECK(engine.isPieceResting(1) == false);

        MoveValidation third = engine.requestMove(Position(5, 4), Position(4, 4));
        CHECK(third.isValid == true);
    }

    SUBCASE("Ambush capture puts the defending piece into a short rest") {
        Piece defender(2, PieceType::Pawn, PieceColor::Black, Position(5, 3));
        board.addPiece(defender);

        GameState state(board);
        GameEngine engine(state);

        engine.requestJump(Position(5, 3));

        // Diagonal capture, pathLength 1 -> same 1000ms duration as the jump,
        // started at the same currentTime, so both complete together.
        engine.requestMove(Position(6, 4), Position(5, 3));
        engine.advanceTime(1000);

        CHECK(engine.hasPieceAt(Position(6, 4)) == false); // mover was captured by the ambush
        CHECK(engine.hasPieceAt(Position(5, 3)) == true);  // defender still standing
        CHECK(engine.isPieceResting(2) == true);           // defender rests briefly after ambushing

        // Black pawns move toward increasing rows, so (6,3) is forward/legal.
        MoveValidation duringRest = engine.requestMove(Position(5, 3), Position(6, 3));
        CHECK(duringRest.isValid == false);
        CHECK(duringRest.reason == MoveValidationReason::PieceResting);

        // matches GameEngine::JUMP_REST_DURATION_MILLIS (private, referenced as
        // a literal the same way REST_DURATION_MILLIS is above)
        engine.advanceTime(1000);
        CHECK(engine.isPieceResting(2) == false);
    }

    SUBCASE("Piece rests briefly after a jump expires without being triggered") {
        GameState state(board);
        GameEngine engine(state);

        engine.requestJump(Position(6, 4));
        engine.advanceTime(1000);

        CHECK(engine.hasActiveJump() == false);
        CHECK(engine.isPieceResting(1) == true);

        MoveValidation duringRest = engine.requestMove(Position(6, 4), Position(5, 4));
        CHECK(duringRest.isValid == false);
        CHECK(duringRest.reason == MoveValidationReason::PieceResting);

        engine.advanceTime(1000);
        CHECK(engine.isPieceResting(1) == false);
    }
}
