#include "tests/doctest.h"

#include "services/RatingService.h"
#include "services/EloCalculator.h"
#include "persistence/Database.h"
#include "persistence/UserRepository.h"

TEST_CASE("Testing RatingService")
{
    Database db(":memory:");
    UserRepository users(db);
    RatingService rating(users);

    SUBCASE("applyGameResult persists both players' post-game ELO ratings")
    {
        const UserRecord winner = users.createUser("winner", "hash");
        const UserRecord loser = users.createUser("loser", "hash");

        const EloCalculator::Outcome expected = EloCalculator::applyResult(winner.score, loser.score);

        rating.applyGameResult(winner.id, loser.id);

        const std::optional<UserRecord> updatedWinner = users.findById(winner.id);
        const std::optional<UserRecord> updatedLoser = users.findById(loser.id);

        REQUIRE(updatedWinner.has_value());
        REQUIRE(updatedLoser.has_value());
        CHECK(updatedWinner->score == expected.newWinnerRating);
        CHECK(updatedLoser->score == expected.newLoserRating);
    }

    SUBCASE("applyGameResult is a no-op if either user id is unknown")
    {
        const UserRecord winner = users.createUser("solo-winner", "hash");

        rating.applyGameResult(winner.id, 999999);

        const std::optional<UserRecord> unchanged = users.findById(winner.id);
        REQUIRE(unchanged.has_value());
        CHECK(unchanged->score == winner.score);
    }
}
