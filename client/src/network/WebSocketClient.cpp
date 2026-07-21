#include "network/WebSocketClient.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <utility>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

struct WebSocketClient::Impl
{
    ix::WebSocket webSocket;
    MessageHandler onMessage;

    std::mutex connectionMutex;
    std::condition_variable connectionCv;
    bool connected = false;
};

WebSocketClient::WebSocketClient(const std::string& url, MessageHandler onMessage)
    : impl(std::make_unique<Impl>())
{
    // Required on Windows (Winsock init) before any socket use; a harmless
    // no-op on other platforms. Paired with uninitNetSystem() in the destructor.
    ix::initNetSystem();

    impl->onMessage = std::move(onMessage);
    impl->webSocket.setUrl(url);
    impl->webSocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Message)
            {
                impl->onMessage(msg->str);
            }
            else if (msg->type == ix::WebSocketMessageType::Open)
            {
                std::lock_guard<std::mutex> lock(impl->connectionMutex);
                impl->connected = true;
                impl->connectionCv.notify_all();
            }
            else if (msg->type == ix::WebSocketMessageType::Close)
            {
                std::lock_guard<std::mutex> lock(impl->connectionMutex);
                impl->connected = false;
            }
        });
}

WebSocketClient::~WebSocketClient()
{
    stop();
    ix::uninitNetSystem();
}

void WebSocketClient::start()
{
    impl->webSocket.start();
}

bool WebSocketClient::waitForConnection(int timeoutMs)
{
    std::unique_lock<std::mutex> lock(impl->connectionMutex);
    return impl->connectionCv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                        [this] { return impl->connected; });
}

void WebSocketClient::send(const std::string& requestJson)
{
    impl->webSocket.send(requestJson);
}

void WebSocketClient::stop()
{
    impl->webSocket.stop();
}
