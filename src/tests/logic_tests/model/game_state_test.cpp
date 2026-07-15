
#include "tests/doctest.h"
#include "src/logic/model/GameState.h"

TEST_CASE("Testing GameState class functionality") {
    // יצירת לוח התחלתי לבדיקות
    Board initialBoard(8, 8);
    GameState state(initialBoard);

    SUBCASE("Initialization") {
        // בדיקה שהלוח הועתק בצורה תקינה
        CHECK(state.getBoard().getRows() == 8);
        CHECK(state.getBoard().getCols() == 8);
        // בדיקה שהמשחק לא מסתיים בהתחלה
        CHECK(state.isGameOver() == false);
    }

    SUBCASE("Game over status") {
        state.setGameOver(true);
        CHECK(state.isGameOver() == true);
        
        state.setGameOver(false);
        CHECK(state.isGameOver() == false);
    }

    SUBCASE("Board modification through GameState") {
        // בדיקה שניתן לגשת ללוח ולבצע בו שינויים
        Board& boardRef = state.getBoard();
        Position pos(0, 0);
        Piece p(1, PieceType::Pawn, PieceColor::White, pos);
        
        boardRef.addPiece(p);
        
        // בדיקה שהשינוי משתקף ב-GameState
        CHECK(state.getBoard().containsPiece(pos) == true);
        CHECK(state.getBoard().getPiece(pos)->getId() == 1);
    }
}