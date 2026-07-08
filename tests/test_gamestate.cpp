#include "doctest.h"

#include "../GameState.h"

static GameState createGame(std::vector<std::vector<Piece>> grid)
{
    Board board(grid);
    return GameState(board);
}

TEST_CASE("Clicking empty square changes nothing")
{
    GameState game = createGame(
    {
        {Piece("."), Piece(".")},
        {Piece("."), Piece(".")}
    });

    game.handleClick(0,0);

    CHECK_NOTHROW(game.printBoard());
}

TEST_CASE("Click outside board")
{
    GameState game = createGame(
    {
        {Piece("wK")}
    });

    game.handleClick(500,500);

    CHECK_NOTHROW(game.printBoard());
}

TEST_CASE("Wait without pending move")
{
    GameState game = createGame(
    {
        {Piece("wK")}
    });

    game.wait(1000);

    CHECK_NOTHROW(game.printBoard());
}

TEST_CASE("Jump outside board")
{
    GameState game = createGame(
    {
        {Piece("wK")}
    });

    game.jump(1000,1000);

    CHECK_NOTHROW(game.printBoard());
}

TEST_CASE("Jump on empty square")
{
    GameState game = createGame(
    {
        {Piece(".")}
    });

    game.jump(0,0);

    CHECK_NOTHROW(game.printBoard());
}

TEST_CASE("Simple rook move")
{
    GameState game = createGame(
    {
        {Piece("wR"), Piece(".")}
    });

    game.handleClick(0,0);
    game.handleClick(100,0);

    game.wait(1000);

    CHECK_NOTHROW(game.printBoard());
}

TEST_CASE("Illegal rook move")
{
    GameState game = createGame(
    {
        {
            Piece("wR"),
            Piece("."),
            Piece(".")
        },
        {
            Piece("."),
            Piece("."),
            Piece(".")
        }
    });

    game.handleClick(0,0);
    game.handleClick(100,100);

    CHECK_NOTHROW(game.printBoard());
}