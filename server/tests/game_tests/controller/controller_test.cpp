#include "tests/doctest.h"
#include "game/Controller/Controller.h"
#include "game/Engine/GameEngine.h"

TEST_CASE("Testing Controller interaction") {
    Board board(8, 8);

    // נשים כלי על הלוח (לפני בניית GameState/GameEngine - ראו הערה ב-game_engine_test.cpp)
    Piece p1(1, PieceType::Pawn, PieceColor::White, Position(6, 4));
    board.addPiece(p1);

    SUBCASE("Selecting and moving") {
        GameState state(board);
        EventBus eventBus;
        GameEngine engine(state, eventBus);
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
        EventBus eventBus;
        GameEngine engine(state, eventBus);
        Controller controller(engine);

        controller.click(Position(6, 4)); // בחרתי את הראשון
        controller.click(Position(5, 5)); // לחצתי על השני (באותו צבע)

        CHECK(controller.getSelectedPosition() == Position(5, 5)); // הבחירה התעדכנה
    }

    SUBCASE("getGameView reflects selection state") {
        GameState state(board);
        EventBus eventBus;
        GameEngine engine(state, eventBus);
        Controller controller(engine);

        CHECK(controller.getGameView().getHasSelection() == false);

        controller.click(Position(6, 4));

        CHECK(controller.getGameView().getHasSelection() == true);
        CHECK(controller.getGameView().getSelectedPosition().getRow() == 6);
        CHECK(controller.getGameView().getSelectedPosition().getCol() == 4);
    }

    SUBCASE("move() is a direct from/to command, independent of click()'s selection state") {
        GameState state(board);
        EventBus eventBus;
        GameEngine engine(state, eventBus);
        Controller controller(engine);

        // No prior click()/selection at all -- move() must still work, and
        // must not leave any selection state behind either.
        controller.move(Position(6, 4), Position(5, 4));

        CHECK(controller.hasSelectedPiece() == false);
        CHECK(engine.hasActiveMotion() == true);
    }
}
