#include "../../../doctest.h"
#include "../../../src/controller/RealTimeArbiter.h"

TEST_CASE("Testing RealTimeArbiter") {
    RealTimeArbiter arbiter;
    Position start(0, 0), end(0, 1);

    SUBCASE("Motion timing") {
        arbiter.startMotion(start, end, 1000);
        CHECK(arbiter.hasActiveMotion() == true);
        CHECK(arbiter.shouldFinishCurrentMotion() == false);
        
        arbiter.advanceTime(1000);
        CHECK(arbiter.shouldFinishCurrentMotion() == true);
        
        arbiter.finishMotion();
        CHECK(arbiter.hasActiveMotion() == false);
    }
}