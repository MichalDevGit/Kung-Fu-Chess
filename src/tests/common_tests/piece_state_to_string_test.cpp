
#include "tests/doctest.h"
#include "src/common/enums/PieceStateToString.h"

TEST_CASE("Testing pieceStateToString") {
    SUBCASE("Idle maps to idle") {
        CHECK(pieceStateToString(PieceState::Idle) == "idle");
    }

    SUBCASE("Jump maps to jump") {
        CHECK(pieceStateToString(PieceState::Jump) == "jump");
    }

    SUBCASE("Moving maps to move") {
        CHECK(pieceStateToString(PieceState::Moving) == "move");
    }

    SUBCASE("Captured maps to captured") {
        CHECK(pieceStateToString(PieceState::Captured) == "captured");
    }

    SUBCASE("LongRest maps to long_rest") {
        CHECK(pieceStateToString(PieceState::LongRest) == "long_rest");
    }

    SUBCASE("ShortRest maps to short_rest") {
        CHECK(pieceStateToString(PieceState::ShortRest) == "short_rest");
    }
}
