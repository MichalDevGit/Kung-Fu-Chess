#include "tests/doctest.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"

using namespace protocol;

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

TEST_CASE("LoginWithTokenRequest round-trips through JSON")
{
    LoginWithTokenRequest request{"header.signature"};

    nlohmann::json j = nlohmann::json::parse(request.toJson());

    CHECK(readType(j) == MessageType::LoginWithToken);

    LoginWithTokenRequest parsed = LoginWithTokenRequest::fromJson(j);
    CHECK(parsed.token == "header.signature");
}

TEST_CASE("LoginResult round-trips its optional token field")
{
    SUBCASE("a successful login carrying a token (API Gateway shape)")
    {
        LoginResult result{true, 7, 1200, "", "header.signature"};

        nlohmann::json j = nlohmann::json::parse(result.toJson());
        LoginResult parsed = LoginResult::fromJson(j);

        CHECK(parsed.success);
        CHECK(parsed.userId == 7);
        CHECK(parsed.score == 1200);
        CHECK(parsed.token == "header.signature");
    }

    SUBCASE("a successful login with no token (WebSocket process's own reply)")
    {
        LoginResult result{true, 7, 1200, "", ""};

        nlohmann::json j = nlohmann::json::parse(result.toJson());
        CHECK_FALSE(j.contains("token"));

        LoginResult parsed = LoginResult::fromJson(j);
        CHECK(parsed.token.empty());
    }
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

TEST_CASE("FindGameRequest round-trips through JSON")
{
    FindGameRequest request;

    nlohmann::json j = nlohmann::json::parse(request.toJson());

    CHECK(readType(j) == MessageType::FindGame);
}

TEST_CASE("SearchingResult round-trips through JSON")
{
    SearchingResult result;

    nlohmann::json j = nlohmann::json::parse(result.toJson());

    CHECK(readType(j) == MessageType::Searching);
}

TEST_CASE("MatchFoundResult round-trips through JSON")
{
    PieceView piece(1, PieceType::King, PieceColor::White, PieceState::Idle, PositionView(0, 0));
    BoardView board(1, 1, std::vector<PieceView>{piece});
    MotionView motion(false, PositionView(0, 0), PositionView(0, 0), 0, 0);
    JumpView jump(false, PositionView(0, 0), 0, 0);
    GameView view(board, motion, jump, std::vector<RestView>{}, false, PositionView(0, 0), 0);

    MatchFoundResult result{"session-7", PieceColor::Black, "opponent1", view};

    nlohmann::json j = nlohmann::json::parse(result.toJson());

    CHECK(readType(j) == MessageType::MatchFound);

    MatchFoundResult parsed = MatchFoundResult::fromJson(j);
    CHECK(parsed.sessionId == "session-7");
    CHECK(parsed.color == PieceColor::Black);
    CHECK(parsed.opponentUsername == "opponent1");
    CHECK(parsed.view.getBoard().getRows() == 1);
}

TEST_CASE("NoMatchResult round-trips through JSON")
{
    NoMatchResult result;

    nlohmann::json j = nlohmann::json::parse(result.toJson());

    CHECK(readType(j) == MessageType::NoMatch);
}

TEST_CASE("OpponentDisconnectedMessage and OpponentReconnectedMessage round-trip through JSON")
{
    nlohmann::json disconnected = nlohmann::json::parse(OpponentDisconnectedMessage{}.toJson());
    CHECK(readType(disconnected) == MessageType::OpponentDisconnected);

    nlohmann::json reconnected = nlohmann::json::parse(OpponentReconnectedMessage{}.toJson());
    CHECK(readType(reconnected) == MessageType::OpponentReconnected);
}
