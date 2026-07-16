#include "UI/BoardCanvas.h"
#include "UI/SpriteManager.h"
#include "UI/Renderer.h"
#include "UI/GameLoop.h"
#include "logic/Controller/Controller.h"
#include "logic/IO/GameFactory.h"
#include <iostream>

int main() {
    try {
        GameEngine engine = GameFactory::createNewGame();
        Controller controller(engine);

        BoardCanvas canvas("assets/board_classic.png", 100);
        SpriteManager spriteManager("assets", "pieces3", 100);
        Renderer renderer(canvas, spriteManager);

        GameLoop gameLoop(controller, renderer, canvas);
        gameLoop.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
