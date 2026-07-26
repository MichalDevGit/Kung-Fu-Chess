// GUI client entry point: connects to the server over ws://, joins the
// (single, implicit) game session, and runs the live OpenCV render/input
// loop against it. Replaces the old local-hotseat src/main.cpp (retired
// once this networked path was proven working) -- GameLoop/Renderer/
// BoardCanvas/SpriteManager/AnimationFrame are otherwise unchanged from
// that version; only what GameLoop talks to (GameClient instead of a
// same-process Controller) is different.
#include <iostream>
#include <string>

#include "ui/BoardCanvas.h"
#include "ui/SpriteManager.h"
#include "ui/AnimationFrame.h"
#include "ui/Renderer.h"
#include "ui/GameLoop.h"
#include "game/GameClient.h"
#include "network/WebSocketClient.h"
#include "common/Config/BoardConfig.h"
#include "common/Config/NetworkConfig.h"

int main()
{
    try
    {
        const std::string url =
            std::string("ws://") + NetworkConfig::DEFAULT_HOST + ":" + std::to_string(NetworkConfig::DEFAULT_PORT);

        // The handler passed here is only a placeholder -- GameClient's
        // constructor immediately replaces it via WebSocketClient::setOnMessage
        // once it exists, since it can't exist before this WebSocketClient does.
        WebSocketClient client(url, [](const std::string&) {});
        client.start();

        std::cout << "Connecting to " << url << " ...\n";

        if (!client.waitForConnection(5000))
        {
            std::cerr << "Failed to connect to " << url << " within 5s -- is the server running?\n";
            return 1;
        }

        std::cout << "Connected.\n";

        GameClient gameClient(client);

        BoardCanvas canvas("assets/board_classic.png", BoardConfig::CELL_SIZE);
        SpriteManager spriteManager("assets", "pieces3", BoardConfig::CELL_SIZE);
        AnimationFrame animationFrame(canvas);
        Renderer renderer(canvas, spriteManager, animationFrame);

        GameLoop gameLoop(gameClient, renderer, canvas);
        gameLoop.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
