#include "GameEngine.h"
#include "../../common/enums/RestKind.h"

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

    if (arbiter.isPieceResting(piece->getId()))
    {
        return MoveValidation(
            false,
            MoveValidationReason::PieceResting);
    }

    int pathLength =
        calculatePathLength(*piece, from, to);

    long long duration =
        static_cast<long long>(pathLength) * MILLIS_PER_SQUARE;

    arbiter.startMotion(from, to, duration);

    return validation;
}

MoveValidation GameEngine::requestJump(
    const Position& position)
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

    Piece* piece =
        getBoard().getPiece(position);

    if (piece == nullptr)
    {
        return MoveValidation(
            false,
            MoveValidationReason::SourceEmpty);
    }

    if (arbiter.hasActiveJump())
    {
        return MoveValidation(
            false,
            MoveValidationReason::MoveAlreadyInProgress);
    }

    if (arbiter.isPieceResting(piece->getId()))
    {
        return MoveValidation(
            false,
            MoveValidationReason::PieceResting);
    }

    arbiter.startJump(position, MILLIS_PER_SQUARE);
    
    return MoveValidation(
        true,
        MoveValidationReason::Valid);
}

void GameEngine::executeMove(const Motion& motion)
{
    if (arbiter.hasActiveJump())
    {
        const Jump& jump =
            arbiter.getCurrentJump();
    
        if (motion.getTo() == jump.getPosition())
        {
            Piece* jumpingPiece =
                getBoard().getPiece(jump.getPosition());
    
            Piece* movingPiece =
                getBoard().getPiece(motion.getFrom());
    
            if (jumpingPiece != nullptr &&
                movingPiece != nullptr &&
                jumpingPiece->getColor() != movingPiece->getColor())
            {
                int jumpingPieceId = jumpingPiece->getId();

                getBoard().removePiece(motion.getFrom());

                arbiter.finishJump();

                arbiter.startRest(jumpingPieceId, JUMP_REST_DURATION_MILLIS, RestKind::Short);

                return;
            }
        }
    }

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

    settleCompletedJumps();

    settleCompletedRests();
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

    const Motion motion = arbiter.getCurrentMotion();

    const Piece* moverBefore = getBoard().getPiece(motion.getFrom());
    int moverId = (moverBefore != nullptr) ? moverBefore->getId() : -1;

    executeMove(motion);

    arbiter.finishMotion();

    if (moverId != -1)
    {
        const Piece* moverAfter = getBoard().getPieceById(moverId);

        if (moverAfter != nullptr && moverAfter->getPosition() == motion.getTo())
        {
            arbiter.startRest(moverId, REST_DURATION_MILLIS, RestKind::Long);
        }
    }
}

void GameEngine::settleCompletedJumps()
{
    if (!arbiter.hasActiveJump())
    {
        return;
    }

    if (!arbiter.shouldFinishCurrentJump())
    {
        return;
    }

    const Jump jump = arbiter.getCurrentJump();

    const Piece* jumpingPiece = getBoard().getPiece(jump.getPosition());
    int jumpingPieceId = (jumpingPiece != nullptr) ? jumpingPiece->getId() : -1;

    arbiter.finishJump();

    if (jumpingPieceId != -1)
    {
        arbiter.startRest(jumpingPieceId, JUMP_REST_DURATION_MILLIS, RestKind::Short);
    }
}

void GameEngine::settleCompletedRests()
{
    arbiter.purgeExpiredRests();
}

const GameState& GameEngine::getGameState() const
{
    return gameState;
}

bool GameEngine::hasActiveMotion() const{
    return arbiter.hasActiveMotion();
}

bool GameEngine::hasActiveJump() const
{
    return arbiter.hasActiveJump();
}

const Motion& GameEngine::getCurrentMotion() const
{
    return arbiter.getCurrentMotion();
}

const Jump& GameEngine::getCurrentJump() const
{
    return arbiter.getCurrentJump();
}

bool GameEngine::isPieceResting(int pieceId) const
{
    return arbiter.isPieceResting(pieceId);
}

std::vector<Rest> GameEngine::getActiveRests() const
{
    return arbiter.getActiveRests();
}

long long GameEngine::getCurrentTime() const{
    return arbiter.getCurrentTime();
}