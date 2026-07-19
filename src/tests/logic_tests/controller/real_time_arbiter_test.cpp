#include "tests/doctest.h"
#include "src/logic/Controller/RealTimeArbiter.h"

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

    SUBCASE("Rest timing per piece") {
        arbiter.startRest(1, 1000);

        CHECK(arbiter.isPieceResting(1) == true);
        CHECK(arbiter.isPieceResting(2) == false); // per-piece, not global
        CHECK(arbiter.getActiveRests().size() == 1);

        arbiter.advanceTime(1000);
        CHECK(arbiter.isPieceResting(1) == false); // defensive check works even before purge

        arbiter.purgeExpiredRests();
        CHECK(arbiter.getActiveRests().empty() == true);
    }
}