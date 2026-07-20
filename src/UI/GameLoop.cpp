#include "GameLoop.h"

#include "../common/PixelPosition.h"

#include <opencv2/opencv.hpp>
#include <chrono>

namespace
{
constexpr int ESCAPE_KEY = 27;
}

GameLoop::GameLoop(Controller& controller, Renderer& renderer, BoardCanvas& canvas)
    : controller(controller),
      renderer(renderer),
      canvas(canvas)
{
}

void GameLoop::run()
{
    cv::namedWindow(canvas.getWindowName(), cv::WINDOW_NORMAL);
    cv::setMouseCallback(canvas.getWindowName(), &GameLoop::mouseCallback, this);

    auto lastTick = std::chrono::steady_clock::now();

    while (true)
    {
        auto now = std::chrono::steady_clock::now();

        long long deltaMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count();

        lastTick = now;

        controller.wait(deltaMs);

        renderer.render(controller.getGameView());

        if (controller.isGameOver())
        {
            renderer.renderGameOver();
        }

        int key = cv::waitKey(1);

        if (key == ESCAPE_KEY )
        {
            break;
        }
    }

    cv::destroyAllWindows();
}

void GameLoop::mouseCallback(int event, int x, int y, int flags, void* userdata)
{
    GameLoop* self = static_cast<GameLoop*>(userdata);

    if (event == cv::EVENT_LBUTTONDOWN)
    {
        self->onMouseDown(x, y);
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        self->onMouseRightDown(x, y);
    }
}

void GameLoop::onMouseDown(int x, int y)
{
    controller.handlePixelClick(PixelPosition(x, y));
}

void GameLoop::onMouseRightDown(int x, int y)
{
    controller.handlePixelJump(PixelPosition(x, y));
}
