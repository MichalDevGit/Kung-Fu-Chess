#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <cstdlib>
#include <vector>
#include "../model/GameState.h"
#include "../model/Rest.h"
#include "../rules/RuleEngine.h"
#include "../../common/DTO/MoveValidation.h"
#include "../Controller/RealTimeArbiter.h"
#include "../../common/enums/PieceType.h"

class GameEngine
{
private:
    GameState gameState;
    RuleEngine ruleEngine;
    RealTimeArbiter arbiter;
    static constexpr long long MILLIS_PER_SQUARE = 1000;
    static constexpr long long REST_DURATION_MILLIS = 2000;
    static constexpr long long JUMP_REST_DURATION_MILLIS = 1000;

    Board& getBoard();

    const Board& getBoard() const;

    void settleCompletedMotions();

    void settleCompletedJumps();

    void settleCompletedRests();

    int calculatePathLength(
        const Piece& piece,
        const Position& from,
        const Position& to) const;

public:
    GameEngine(const GameState& gameState);

    void advanceTime(long long milliseconds);

    MoveValidation requestMove(
        const Position& from,
        const Position& to);

    MoveValidation requestJump(
        const Position& position);

    void executeMove(const Motion& motion);

    const GameState& getGameState() const;

    bool hasPieceAt(const Position& position) const;

    bool hasActiveMotion() const;

    bool hasActiveJump() const;

    const Motion& getCurrentMotion() const;

    const Jump& getCurrentJump() const;

    bool isPieceResting(int pieceId) const;

    std::vector<Rest> getActiveRests() const;

    long long getCurrentTime() const;
};

#endif