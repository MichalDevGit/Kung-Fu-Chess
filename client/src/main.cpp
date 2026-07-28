// KungFuChess client entry point: a single process that authenticates over
// an interactive CLI (register/login), then -- once logged in -- either
// resumes an in-progress game (reconnect) or is automatically queued for
// matchmaking, and finally opens the OpenCV game pane against the very same
// authenticated WebSocketClient connection. There is deliberately no
// separate "GUI client" executable/process anymore (see the retired
// gui_main.cpp/KungFuChessGuiClient) and no second connection or re-auth
// step -- identity lives on this one connection for its whole lifetime.
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "cli/CliShell.h"
#include "network/ApiGatewayClient.h"
#include "network/WebSocketClient.h"
#include "game/GameClient.h"
#include "ui/BoardCanvas.h"
#include "ui/SpriteManager.h"
#include "ui/AnimationFrame.h"
#include "ui/Renderer.h"
#include "ui/GameLoop.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "common/Config/NetworkConfig.h"
#include "common/Config/BoardConfig.h"
#include "common/enums/PieceColor.h"

namespace
{
// Shared between the WebSocketClient's background message-callback thread
// and this file's main-thread matchmaking wait -- lets main() block on
// "the next match_found/no_match has arrived" instead of polling, same
// synchronization shape CliShell already uses to wait for login_result.
struct MatchmakingSync
{
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<protocol::MatchFoundResult> matchFound;
    bool noMatch = false;
};

// Blocks (auto-retrying on a NoMatchResult, with the user's confirmation)
// until a MatchFoundResult arrives, then returns it. Installs its own
// message handler on `client` for the duration -- replaced again by
// GameClient once this returns (see main() below).
protocol::MatchFoundResult waitForMatch(WebSocketClient& client, std::mutex& outputMutex)
{
    MatchmakingSync sync;

    client.setOnMessage(
        [&](const std::string& json)
        {
            try
            {
                const nlohmann::json parsed = nlohmann::json::parse(json);
                const std::string type = protocol::readType(parsed);

                if (type == protocol::MessageType::MatchFound)
                {
                    std::lock_guard<std::mutex> lock(sync.mutex);
                    sync.matchFound = protocol::MatchFoundResult::fromJson(parsed);
                    sync.cv.notify_all();
                }
                else if (type == protocol::MessageType::NoMatch)
                {
                    std::lock_guard<std::mutex> lock(sync.mutex);
                    sync.noMatch = true;
                    sync.cv.notify_all();
                }
            }
            catch (const nlohmann::json::exception&)
            {
                // Same never-throw-out-of-a-socket-callback philosophy as
                // GameClient::onMessage/CliShell::onMessage.
            }
        });

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Searching for an opponent (up to a minute)...\n";
        }

        client.send(protocol::FindGameRequest{}.toJson());

        std::unique_lock<std::mutex> lock(sync.mutex);
        sync.cv.wait(lock, [&] { return sync.matchFound.has_value() || sync.noMatch; });

        if (sync.matchFound.has_value())
            return *sync.matchFound;

        sync.noMatch = false;
        lock.unlock();

        std::lock_guard<std::mutex> outLock(outputMutex);
        std::cout << "No opponent found within a minute -- searching again.\n";
    }
}

void runGame(WebSocketClient& client, const protocol::MatchFoundResult& match)
{
    std::cout << "Match found! You are " << (match.color == PieceColor::White ? "White" : "Black")
               << " against " << match.opponentUsername << ".\n";

    GameClient gameClient(client, match.view);

    BoardCanvas canvas("assets/board_classic.png", BoardConfig::CELL_SIZE);
    SpriteManager spriteManager("assets", "pieces3", BoardConfig::CELL_SIZE);
    AnimationFrame animationFrame(canvas);
    Renderer renderer(canvas, spriteManager, animationFrame);

    GameLoop gameLoop(gameClient, renderer, canvas);
    gameLoop.run();
}
}

int main()
{
    try
    {
        const std::string url =
            std::string("ws://") + NetworkConfig::DEFAULT_HOST + ":" + std::to_string(NetworkConfig::DEFAULT_PORT);
        std::mutex outputMutex;

        WebSocketClient client(url, [&outputMutex](const std::string& responseJson)
                                {
                                    std::lock_guard<std::mutex> lock(outputMutex);
                                    std::cout << "\n[server] " << responseJson << "\n> " << std::flush;
                                });

        std::cout << "Connecting to " << url << " ...\n";
        client.start();

        if (!client.waitForConnection(5000))
        {
            std::cout << "Failed to connect to " << url << " within 5s -- is the server running?\n";
            return 1;
        }
        std::cout << "Connected.\n";

        const std::string apiGatewayUrl =
            std::string("http://") + NetworkConfig::API_GATEWAY_HOST + ":" + std::to_string(NetworkConfig::API_GATEWAY_PORT);
        ApiGatewayClient apiGatewayClient(apiGatewayUrl);

        CliShell shell(client, apiGatewayClient, outputMutex);
        const CliShell::LoginOutcome outcome = shell.run();

        if (!outcome.loggedIn)
            return 0; // user quit before logging in

        std::cout << "Logged in as " << outcome.username << " (score " << outcome.score << ").\n";

        // A reconnecting player already has their resume waiting (see
        // CliShell::LoginOutcome::resumedMatch) -- skip matchmaking
        // entirely and go straight back into the same game.
        const protocol::MatchFoundResult match =
            outcome.resumedMatch.has_value() ? *outcome.resumedMatch : waitForMatch(client, outputMutex);

        runGame(client, match);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
