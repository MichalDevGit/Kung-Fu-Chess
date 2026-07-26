#include "GameLoop.h"

#include "../../../common/PixelPosition.h"

#include <opencv2/opencv.hpp>

namespace
{
constexpr int ESCAPE_KEY = 27;
}

GameLoop::GameLoop(GameClient& gameClient, Renderer& renderer, BoardCanvas& canvas)
    : gameClient(gameClient),
      renderer(renderer),
      canvas(canvas)
{
}

void GameLoop::run()
{
    cv::namedWindow(canvas.getWindowName(), cv::WINDOW_NORMAL);
    cv::setMouseCallback(canvas.getWindowName(), &GameLoop::mouseCallback, this);

    while (true)
    {
        renderer.render(gameClient.getGameView());

        if (gameClient.isGameOver())
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
    gameClient.handlePixelClick(PixelPosition(x, y));
}

void GameLoop::onMouseRightDown(int x, int y)
{
    gameClient.handlePixelJump(PixelPosition(x, y));
}
