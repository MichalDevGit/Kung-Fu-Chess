#include "network/WebSocketServer.h"

#include <stdexcept>
#include <utility>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketServer.h>

struct WebSocketServer::Impl
{
    ix::WebSocketServer server;
    RequestHandler handler;

    Impl(int port, RequestHandler handler)
        : server(port, "127.0.0.1")
        , handler(std::move(handler))
    {
    }
};

WebSocketServer::WebSocketServer(int port, RequestHandler handler)
    : impl(std::make_unique<Impl>(port, std::move(handler)))
{
    // Required on Windows (Winsock init) before any socket use; a harmless
    // no-op on other platforms. Paired with uninitNetSystem() in the destructor.
    ix::initNetSystem();

    impl->server.setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState>, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type != ix::WebSocketMessageType::Message)
                return;

            const std::string response = impl->handler(msg->str);
            webSocket.send(response);
        });
}

WebSocketServer::~WebSocketServer()
{
    stop();
    ix::uninitNetSystem();
}

void WebSocketServer::start()
{
    const auto result = impl->server.listen();
    if (!result.first)
        throw std::runtime_error("WebSocketServer failed to listen: " + result.second);

    impl->server.start();
}

void WebSocketServer::wait()
{
    impl->server.wait();
}

void WebSocketServer::stop()
{
    impl->server.stop();
}
