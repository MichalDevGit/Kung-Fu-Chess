
#include "tests/doctest.h"
#include "src/common/enums/PieceStateToString.h"

TEST_CASE("Testing pieceStateToString") {
    SUBCASE("Idle maps to idle") {
        CHECK(pieceStateToString(PieceState::Idle) == "idle");
    }

    SUBCASE("Jump maps to jump") {
        CHECK(pieceStateToString(PieceState::Jump) == "jump");
    }

    SUBCASE("Captured maps to captured") {
        CHECK(pieceStateToString(PieceState::Captured) == "captured");
    }
}
