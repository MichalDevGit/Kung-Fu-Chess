#include "tests/doctest.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"

using namespace protocol;

TEST_CASE("JoinGameRequest round-trips through JSON")
{
    JoinGameRequest request;

    nlohmann::json j = nlohmann::json::parse(request.toJson());

    CHECK(readType(j) == MessageType::JoinGame);

    JoinGameRequest parsed = JoinGameRequest::fromJson(j);
    (void)parsed;
}

TEST_CASE("GameJoinedResult round-trips through JSON")
{
    GameJoinedResult result{"session-42"};

    nlohmann::json j = nlohmann::json::parse(result.toJson());

    CHECK(readType(j) == MessageType::GameJoined);

    GameJoinedResult parsed = GameJoinedResult::fromJson(j);
    CHECK(parsed.sessionId == "session-42");
}

TEST_CASE("MoveRequest round-trips through JSON")
{
    MoveRequest request{6, 4, 5, 4};

    nlohmann::json j = nlohmann::json::parse(request.toJson());

    CHECK(readType(j) == MessageType::Move);

    MoveRequest parsed = MoveRequest::fromJson(j);
    CHECK(parsed.fromRow == 6);
    CHECK(parsed.fromCol == 4);
    CHECK(parsed.toRow == 5);
    CHECK(parsed.toCol == 4);
}

TEST_CASE("JumpRequest round-trips through JSON")
{
    JumpRequest request{3, 7};

    nlohmann::json j = nlohmann::json::parse(request.toJson());

    CHECK(readType(j) == MessageType::Jump);

    JumpRequest parsed = JumpRequest::fromJson(j);
    CHECK(parsed.row == 3);
    CHECK(parsed.col == 7);
}

TEST_CASE("GameStartedMessage round-trips through JSON")
{
    GameStartedMessage message;

    nlohmann::json j = nlohmann::json::parse(message.toJson());

    CHECK(readType(j) == MessageType::GameStarted);
}

TEST_CASE("GameOverMessage round-trips through JSON")
{
    GameOverMessage message;

    nlohmann::json j = nlohmann::json::parse(message.toJson());

    CHECK(readType(j) == MessageType::GameOver);
}

TEST_CASE("GameViewMessage round-trips a full GameView snapshot through JSON")
{
    // A 1x1 board so getPiece(row, col)'s row*cols+col indexing lines up with
    // a single-element pieces vector without needing to pad out a dense 8x8.
    PieceView piece(7, PieceType::Queen, PieceColor::White, PieceState::Idle, PositionView(0, 0));
    BoardView board(1, 1, std::vector<PieceView>{piece});

    MotionView motion(true, PositionView(6, 4), PositionView(5, 4), 1000, 2000);
    JumpView jump(false, PositionView(0, 0), 0, 0);

    std::vector<RestView> rests{
        RestView(7, PositionView(0, 0), 500, 1500, RestKind::Short)};

    GameView view(board, motion, jump, rests, true, PositionView(0, 0), 1234);

    GameViewMessage message{view};

    nlohmann::json j = nlohmann::json::parse(message.toJson());

    CHECK(readType(j) == MessageType::GameView);

    GameViewMessage parsed = GameViewMessage::fromJson(j);

    CHECK(parsed.view.getBoard().getRows() == 1);
    CHECK(parsed.view.getBoard().getCols() == 1);
    CHECK(parsed.view.getBoard().getPiece(0, 0).getId() == 7);
    CHECK(parsed.view.getBoard().getPiece(0, 0).getType() == PieceType::Queen);
    CHECK(parsed.view.getBoard().getPiece(0, 0).getColor() == PieceColor::White);

    CHECK(parsed.view.getMotion().isActive() == true);
    CHECK(parsed.view.getMotion().getFrom().getRow() == 6);
    CHECK(parsed.view.getMotion().getTo().getRow() == 5);
    CHECK(parsed.view.getMotion().getStartTime() == 1000);
    CHECK(parsed.view.getMotion().getEndTime() == 2000);

    CHECK(parsed.view.getJump().isActive() == false);

    REQUIRE(parsed.view.getRests().size() == 1);
    CHECK(parsed.view.getRests()[0].getPieceId() == 7);
    CHECK(parsed.view.getRests()[0].getKind() == RestKind::Short);

    CHECK(parsed.view.getHasSelection() == true);
    CHECK(parsed.view.getSelectedPosition().getRow() == 0);
    CHECK(parsed.view.getSelectedPosition().getCol() == 0);
    CHECK(parsed.view.getCurrentTime() == 1234);
}
