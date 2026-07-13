#include "doctest.h"

#include "..\model\Piece.h"

TEST_CASE("Default constructor creates empty piece")
{
    Piece p;

    CHECK(p.isEmpty());
    CHECK(p.toString() == ".");
}

TEST_CASE("Constructor stores value")
{
    Piece p("wK");

    CHECK(p.toString() == "wK");
    CHECK_FALSE(p.isEmpty());
}

TEST_CASE("getColor")
{
    CHECK(Piece("wQ").getColor() == 'w');
    CHECK(Piece("bR").getColor() == 'b');
    CHECK(Piece(".").getColor() == '\0');
}

TEST_CASE("getType")
{
    CHECK(Piece("wQ").getType() == 'Q');
    CHECK(Piece("bN").getType() == 'N');
    CHECK(Piece(".").getType() == '\0');
}

TEST_CASE("isWhite")
{
    CHECK(Piece("wP").isWhite());
    CHECK_FALSE(Piece("bP").isWhite());
}

TEST_CASE("isBlack")
{
    CHECK(Piece("bP").isBlack());
    CHECK_FALSE(Piece("wP").isBlack());
}

TEST_CASE("setValue")
{
    Piece p;

    p.setValue("bK");

    CHECK(p.toString() == "bK");
    CHECK(p.isBlack());
}

TEST_CASE("Equality operator")
{
    Piece a("wR");
    Piece b("wR");
    Piece c("bR");

    CHECK(a == b);
    CHECK(a != c);
}