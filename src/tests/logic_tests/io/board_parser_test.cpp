#include "../../../doctest.h"
#include "../../../src/IO/BoardParser.h"

TEST_CASE("Testing BoardParser") {
    SUBCASE("Parsing valid board") {
        std::string boardData = "wK . \n . bQ";
        Board board = BoardParser::parse(boardData);
        
        CHECK(board.getRows() == 2);
        CHECK(board.getCols() == 2);
        CHECK(board.getPiece(Position(0, 0))->getType() == PieceType::King);
        CHECK(board.getPiece(Position(1, 1))->getType() == PieceType::Queen);
    }

    SUBCASE("Parsing invalid board") {
        std::string invalidData = "XX XX";
        CHECK_THROWS_AS(BoardParser::parse(invalidData), std::runtime_error);
    }
}