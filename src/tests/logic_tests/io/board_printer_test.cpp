
#include "tests/doctest.h"
#include "src/logic/IO/BoardPrinter.h"

TEST_CASE("Testing BoardPrinter") {
    Board board(1, 2);
    // הוספת מלך לבן
    board.addPiece(Piece(1, PieceType::King, PieceColor::White, Position(0, 0)));
    
    std::string output = BoardPrinter::print(board);
    
    // בודקים שהפלט תואם לפורמט (wK עבור מלך לבן)
    CHECK(output == "wK .");
}