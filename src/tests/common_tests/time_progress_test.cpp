
#include "tests/doctest.h"
#include "src/common/TimeProgress.h"

TEST_CASE("Testing computeProgress") {
    SUBCASE("Before start clamps to 0.0") {
        CHECK(computeProgress(100, 200, 50) == doctest::Approx(0.0));
    }

    SUBCASE("At or after end clamps to 1.0") {
        CHECK(computeProgress(100, 200, 200) == doctest::Approx(1.0));
        CHECK(computeProgress(100, 200, 300) == doctest::Approx(1.0));
    }

    SUBCASE("Midpoint gives 0.5") {
        CHECK(computeProgress(100, 200, 150) == doctest::Approx(0.5));
    }

    SUBCASE("Degenerate zero-duration returns 1.0") {
        CHECK(computeProgress(100, 100, 100) == doctest::Approx(1.0));
    }
}
