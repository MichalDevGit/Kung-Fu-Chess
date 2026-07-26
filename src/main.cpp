#include "UI/BoardCanvas.h"
#include "UI/SpriteManager.h"
#include "UI/AnimationFrame.h"
#include "UI/Renderer.h"
#include "UI/GameLoop.h"
#include "logic/Controller/Controller.h"
#include "logic/IO/GameFactory.h"
#include "../common/EventBus/Events.h"
#include "../common/Config/BoardConfig.h"
#include <iostream>

int main() {
    try {
        EventBus eventBus;
        GameEngine engine = GameFactory::createNewGame(eventBus);
        eventBus.publish(GameStartedEvent{});
        Controller controller(engine);

        BoardCanvas canvas("assets/board_classic.png", BoardConfig::CELL_SIZE);
        SpriteManager spriteManager("assets", "pieces3", BoardConfig::CELL_SIZE);
        AnimationFrame animationFrame(canvas);
        Renderer renderer(canvas, spriteManager, animationFrame);

        GameLoop gameLoop(controller, renderer, canvas);
        gameLoop.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
