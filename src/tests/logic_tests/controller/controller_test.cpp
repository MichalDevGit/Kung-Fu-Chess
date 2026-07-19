#include "tests/doctest.h"
#include "src/logic/Controller/Controller.h"
#include "src/logic/Engine/GameEngine.h"

TEST_CASE("Testing Controller interaction") {
    Board board(8, 8);

    // נשים כלי על הלוח (לפני בניית GameState/GameEngine - ראו הערה ב-game_engine_test.cpp)
    Piece p1(1, PieceType::Pawn, PieceColor::White, Position(6, 4));
    board.addPiece(p1);

    SUBCASE("Selecting and moving") {
        GameState state(board);
        GameEngine engine(state);
        Controller controller(engine);

        // בחירת כלי
        controller.click(Position(6, 4));
        CHECK(controller.hasSelectedPiece() == true);
        CHECK(controller.getSelectedPosition() == Position(6, 4));

        // הזזה למקום חוקי
        controller.click(Position(5, 4));
        CHECK(controller.hasSelectedPiece() == false); // הבחירה התבטלה לאחר המהלך
    }

    SUBCASE("Changing selection") {
        Piece p2(2, PieceType::Pawn, PieceColor::White, Position(5, 5));
        board.addPiece(p2);

        GameState state(board);
        GameEngine engine(state);
        Controller controller(engine);

        controller.click(Position(6, 4)); // בחרתי את הראשון
        controller.click(Position(5, 5)); // לחצתי על השני (באותו צבע)

        CHECK(controller.getSelectedPosition() == Position(5, 5)); // הבחירה התעדכנה
    }

    SUBCASE("getGameView reflects selection state") {
        GameState state(board);
        GameEngine engine(state);
        Controller controller(engine);

        CHECK(controller.getGameView().getHasSelection() == false);

        controller.click(Position(6, 4));

        CHECK(controller.getGameView().getHasSelection() == true);
        CHECK(controller.getGameView().getSelectedPosition().getRow() == 6);
        CHECK(controller.getGameView().getSelectedPosition().getCol() == 4);
    }
}
