#include "GameEngine.h"
#include <cstdlib>

GameEngine::GameEngine(const GameState& gameState)
    : gameState(gameState)
{
}

MoveValidation GameEngine::requestMove(
    const Position& from,
    const Position& to)
{
    if (gameState.isGameOver())
    {
        return MoveValidation(
            false,
            MoveValidationReason::GameOver);
    }

    if (arbiter.hasActiveMotion())
    {
        return MoveValidation(
            false,
            MoveValidationReason::MoveAlreadyInProgress);
    }
    
    MoveValidation validation =
        ruleEngine.validateMove(getBoard(), from, to);

    if (!validation.isValid)
    {
        return validation;
    }

    const Piece* piece = getBoard().getPiece(from);

    int pathLength =
        calculatePathLength(*piece, from, to);

    long long duration =
        static_cast<long long>(pathLength) * MILLIS_PER_SQUARE;

    arbiter.startMotion(from, to, duration);

    return validation;
}

void GameEngine::executeMove(const Motion& motion)
{
    Piece* destinationPiece =
        getBoard().getPiece(motion.getTo());

    bool kingCaptured =
        destinationPiece != nullptr &&
        destinationPiece->getType() == PieceType::King;

    if (destinationPiece != nullptr)
    {
        getBoard().removePiece(motion.getTo());
    }

    getBoard().movePiece(
        motion.getFrom(),
        motion.getTo());

    if (kingCaptured)
    {
        gameState.setGameOver(true);
    }

    Piece* movedPiece =
        getBoard().getPiece(motion.getTo());

    if (movedPiece == nullptr)
    {
        return;
    }

    if (movedPiece->getType() == PieceType::Pawn)
    {
        int row = movedPiece->getPosition().getRow();
        int lastRow = getBoard().getRows() - 1;

        if ((movedPiece->isWhite() && row == 0) ||
            (movedPiece->isBlack() && row == lastRow))
        {
            movedPiece->setType(PieceType::Queen);
        }
    }
}

void GameEngine::advanceTime(long long milliseconds)
{
    if (milliseconds <= 0)
    {
        return;
    }

    arbiter.advanceTime(milliseconds);
    
    settleCompletedMotions();
}

bool GameEngine::hasPieceAt(const Position& position) const
{
    return getBoard().containsPiece(position);
}
        
Board& GameEngine::getBoard()
{
        return gameState.getBoard();
}
        
const Board& GameEngine::getBoard() const
{
    return gameState.getBoard();
}
        
int GameEngine::calculatePathLength(
    const Piece& piece,
    const Position& from,
    const Position& to) const
{
    int rowDistance =
    std::abs(to.getRow() - from.getRow());
                
    int colDistance =
        std::abs(to.getCol() - from.getCol());
                
    switch (piece.getType())
    {
        case PieceType::King:
            return 1;
                    
        case PieceType::Knight:
            return 1;
                    
        case PieceType::Pawn:
            return rowDistance;

        case PieceType::Rook:
            return rowDistance + colDistance;

        case PieceType::Bishop:
            return rowDistance;
        
        case PieceType::Queen:
            if (rowDistance == 0 || colDistance == 0)
            {
                return rowDistance + colDistance;
            }
            return rowDistance;
        
        default:
        return 1;
    }
}

void GameEngine::settleCompletedMotions()
{
    if (!arbiter.hasActiveMotion())
    {
        return;
    }
    
    if (!arbiter.shouldFinishCurrentMotion())
    {
        return;
    }
    
    executeMove(arbiter.getCurrentMotion());
    
    arbiter.finishMotion();
}

const GameState& GameEngine::getGameState() const
{
    return gameState;
}

bool GameEngine::hasActiveMotion() const{
    return arbiter.hasActiveMotion();
}

long long GameEngine::getCurrentTime() const{
    return arbiter.getCurrentTime();
}