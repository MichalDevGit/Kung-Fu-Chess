
#include "tests/doctest.h"
#include "src/logic/model/Jump.h"

TEST_CASE("Testing Jump class functionality") {
    Position p(3, 3);
    Jump j(p, 500, 600);

    SUBCASE("Initialization") {
        CHECK(j.isActive() == true);
        CHECK(j.getPosition() == p);
        CHECK(j.getStartTime() == 500);
        CHECK(j.getEndTime() == 600);
    }

    SUBCASE("Timing and Status") {
        CHECK(j.isFinished(400) == false);
        CHECK(j.isFinished(600) == true);
        CHECK(j.isFinished(700) == true);
    }

    SUBCASE("Activation/Deactivation") {
        j.deactivate();
        CHECK(j.isActive() == false);
        j.activate();
        CHECK(j.isActive() == true);
    }
}