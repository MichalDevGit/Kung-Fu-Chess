class RuleEngine
{
public:
    MoveValidation validateMove(
        const Board& board,
        const Position& from,
        const Position& to) const;

private:
    RookRule rookRule;
};