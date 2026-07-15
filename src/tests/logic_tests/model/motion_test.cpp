
#include "tests/doctest.h"
#include "src/logic/model/Motion.h"

TEST_CASE("Testing Motion class functionality") {
    Position p1(0, 0), p2(1, 1);
    Motion m(p1, p2, 100, 200);

    SUBCASE("Initialization") {
        CHECK(m.isActive() == true);
        CHECK(m.getFrom() == p1);
        CHECK(m.getTo() == p2);
        CHECK(m.getStartTime() == 100);
        CHECK(m.getEndTime() == 200);
    }

    SUBCASE("Timing and Status") {
        CHECK(m.hasStarted(50) == false);
        CHECK(m.hasStarted(150) == true);
        CHECK(m.isFinished(150) == false);
        CHECK(m.isFinished(250) == true);
    }

    SUBCASE("Activation/Deactivation") {
        m.deactivate();
        CHECK(m.isActive() == false);
        m.activate();
        CHECK(m.isActive() == true);
    }
}