#include "../../../doctest.h"
#include "../../../src/controller/Controller.h"
#include "../../../src/logic/engine/GameEngine.h"

TEST_CASE("Testing Controller interaction") {
    Board board(8, 8);
    GameState state(board);
    GameEngine engine(state);
    Controller controller(engine);

    // נשים כלי על הלוח
    Piece p1(1, PieceType::Pawn, PieceColor::White, Position(6, 4));
    board.addPiece(p1);

    SUBCASE("Selecting and moving") {
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

        controller.click(Position(6, 4)); // בחרתי את הראשון
        controller.click(Position(5, 5)); // לחצתי על השני (באותו צבע)
        
        CHECK(controller.getSelectedPosition() == Position(5, 5)); // הבחירה התעדכנה
    }
}