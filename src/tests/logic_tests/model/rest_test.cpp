
#include "tests/doctest.h"
#include "src/logic/model/Rest.h"

TEST_CASE("Testing Rest class functionality") {
    Rest r(1, 100, 200, RestKind::Long);

    SUBCASE("Initialization") {
        CHECK(r.getPieceId() == 1);
        CHECK(r.getStartTime() == 100);
        CHECK(r.getEndTime() == 200);
        CHECK(r.getKind() == RestKind::Long);
    }

    SUBCASE("Timing and Status") {
        CHECK(r.isFinished(150) == false);
        CHECK(r.isFinished(200) == true);
        CHECK(r.isFinished(250) == true);
    }

    SUBCASE("Kind distinguishes post-move from post-jump rest") {
        Rest shortRest(2, 100, 200, RestKind::Short);
        CHECK(shortRest.getKind() == RestKind::Short);
    }
}
