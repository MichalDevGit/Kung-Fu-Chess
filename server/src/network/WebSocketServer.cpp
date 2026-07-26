#include "network/WebSocketServer.h"

#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketServer.h>

struct WebSocketServer::Impl
{
    ix::WebSocketServer server;
    RequestHandler handler;

    // Every currently-open connection, keyed by ConnectionState::getId(), so
    // broadcast() can reach clients that never sent a request at all. Guarded
    // by its own mutex, separate from anything game-related -- this class
    // only ever tracks "who's connected," never game state.
    std::mutex connectionsMutex;
    std::unordered_map<std::string, ix::WebSocket*> connections;

    explicit Impl(int port)
        : server(port, "127.0.0.1")
    {
    }
};

WebSocketServer::WebSocketServer(int port)
    : impl(std::make_unique<Impl>(port))
{
    // Required on Windows (Winsock init) before any socket use; a harmless
    // no-op on other platforms. Paired with uninitNetSystem() in the destructor.
    ix::initNetSystem();

    impl->server.setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Open)
            {
                std::lock_guard<std::mutex> lock(impl->connectionsMutex);
                impl->connections[connectionState->getId()] = &webSocket;
            }
            else if (msg->type == ix::WebSocketMessageType::Close)
            {
                std::lock_guard<std::mutex> lock(impl->connectionsMutex);
                impl->connections.erase(connectionState->getId());
            }
            else if (msg->type == ix::WebSocketMessageType::Message)
            {
                const std::string response = impl->handler(msg->str);
                webSocket.send(response);
            }
        });
}

WebSocketServer::~WebSocketServer()
{
    stop();
    ix::uninitNetSystem();
}

void WebSocketServer::setRequestHandler(RequestHandler handler)
{
    impl->handler = std::move(handler);
}

void WebSocketServer::broadcast(const std::string& json)
{
    std::lock_guard<std::mutex> lock(impl->connectionsMutex);

    for (auto& entry : impl->connections)
    {
        // A connection mid-close by the time this runs shouldn't take the
        // whole broadcast down with it -- Close will erase it from the map
        // on its own callback; this is just a defensive per-send guard.
        try
        {
            entry.second->send(json);
        }
        catch (const std::exception&)
        {
        }
    }
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
