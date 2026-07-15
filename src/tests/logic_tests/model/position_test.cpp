#include "../../doctest.h"
#include "../../../logic/model/Position.h"

TEST_CASE("Testing Position class functionality") {
    
    SUBCASE("Constructors and Getters") {
        Position p1;
        CHECK(p1.getRow() == 0);
        CHECK(p1.getCol() == 0);

        Position p2(3, 4);
        CHECK(p2.getRow() == 3);
        CHECK(p2.getCol() == 4);
    }

    SUBCASE("Setters") {
        Position p;
        p.setRow(5);
        p.setCol(2);
        CHECK(p.getRow() == 5);
        CHECK(p.getCol() == 2);

        p.setPosition(1, 1);
        CHECK(p.getRow() == 1);
        CHECK(p.getCol() == 1);
    }

    SUBCASE("Equality and Inequality operators") {
        Position p1(2, 2);
        Position p2(2, 2);
        Position p3(1, 2);

        CHECK(p1 == p2);
        CHECK(p1 != p3);
        CHECK_FALSE(p1 != p2);
    }

    SUBCASE("Less than operator (for sorting/maps)") {
        Position p1(1, 1);
        Position p2(1, 2);
        Position p3(2, 0);

        CHECK(p1 < p2); 
        CHECK(p2 < p3); 
        CHECK_FALSE(p3 < p1);
    }
}