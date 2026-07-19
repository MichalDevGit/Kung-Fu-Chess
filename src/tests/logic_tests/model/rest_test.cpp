
#include "tests/doctest.h"
#include "src/logic/model/Rest.h"

TEST_CASE("Testing Rest class functionality") {
    Rest r(1, 100, 200);

    SUBCASE("Initialization") {
        CHECK(r.getPieceId() == 1);
        CHECK(r.getStartTime() == 100);
        CHECK(r.getEndTime() == 200);
    }

    SUBCASE("Timing and Status") {
        CHECK(r.isFinished(150) == false);
        CHECK(r.isFinished(200) == true);
        CHECK(r.isFinished(250) == true);
    }
}
