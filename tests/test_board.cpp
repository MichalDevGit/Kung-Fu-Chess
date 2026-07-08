#include "doctest.h"

#include "../Board.h"

TEST_CASE("Board dimensions")
{
    std::vector<std::vector<Piece>> grid =
    {
        {Piece("wK"), Piece(".")},
        {Piece("."), Piece("bK")}
    };

    Board board(grid);

    CHECK(board.getRows() == 2);
    CHECK(board.getCols() == 2);
}

TEST_CASE("Valid positions")
{
    std::vector<std::vector<Piece>> grid(3,
        std::vector<Piece>(4, Piece(".")));

    Board board(grid);

    CHECK(board.isValidPosition(0,0));
    CHECK(board.isValidPosition(2,3));

    CHECK_FALSE(board.isValidPosition(-1,0));
    CHECK_FALSE(board.isValidPosition(3,0));
    CHECK_FALSE(board.isValidPosition(0,4));
}

TEST_CASE("Get piece")
{
    std::vector<std::vector<Piece>> grid =
    {
        {Piece("wQ"), Piece(".")}
    };

    Board board(grid);

    CHECK(board.getPiece(0,0) == Piece("wQ"));
    CHECK(board.getPiece(0,1).isEmpty());
}

TEST_CASE("Invalid getPiece returns empty")
{
    std::vector<std::vector<Piece>> grid =
    {
        {Piece("wQ")}
    };

    Board board(grid);

    CHECK(board.getPiece(5,5).isEmpty());
}

TEST_CASE("setPiece")
{
    std::vector<std::vector<Piece>> grid =
    {
        {Piece(".")}
    };

    Board board(grid);

    board.setPiece(0,0,Piece("bQ"));

    CHECK(board.getPiece(0,0) == Piece("bQ"));
}

TEST_CASE("Invalid setPiece does nothing")
{
    std::vector<std::vector<Piece>> grid =
    {
        {Piece(".")}
    };

    Board board(grid);

    board.setPiece(5,5,Piece("wK"));

    CHECK(board.getPiece(0,0).isEmpty());
}