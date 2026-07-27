#ifndef ELO_CALCULATOR_H
#define ELO_CALCULATOR_H

// Pure, stateless ELO rating math -- no DB, no networking, no locking. Takes
// two current ratings and who won, returns both parties' new ratings. Kept
// entirely separate from RatingService (which knows how to read/persist a
// rating) so the arithmetic itself stays trivially unit-testable and free of
// any IUserRepository dependency.
class EloCalculator
{
public:
    struct Outcome
    {
        int newWinnerRating = 0;
        int newLoserRating = 0;
    };

    // ratingWinner/ratingLoser are the two players' ratings *before* this
    // game. K-factor and the logistic-curve base come from
    // common/Config/RatingConfig.h -- never hardcoded here.
    static Outcome applyResult(int ratingWinner, int ratingLoser);

private:
    // Probability (0..1) that a player rated `rating` beats a player rated
    // `opponentRating`, per the standard ELO logistic curve.
    static double expectedScore(int rating, int opponentRating);

    // A single side's post-game rating: rating + K * (actualScore - expected).
    static int updatedRating(int rating, double expected, double actualScore);
};

#endif
