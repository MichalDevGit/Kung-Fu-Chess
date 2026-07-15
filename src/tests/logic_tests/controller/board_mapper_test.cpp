#include "../../../doctest.h"
#include "../../../src/controller/BoardMapper.h"

TEST_CASE("Testing BoardMapper") {
    BoardMapper mapper;

    SUBCASE("Pixel to cell conversion") {
        // CELL_SIZE = 100
        Position pos = mapper.pixelToCell(150, 250); 
        CHECK(pos.getRow() == 2);
        CHECK(pos.getCol() == 1);
    }
}