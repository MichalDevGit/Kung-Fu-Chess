#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <functional>
#include <memory>
#include <string>

// The only header/source pair in this codebase that includes ixwebsocket's
// server-side headers (see the .cpp). Exposes a plain "JSON request string
// in, JSON response string out" seam so AuthRequestHandler/AuthService never
// need to know a WebSocket -- or ixwebsocket specifically -- is involved at
// all. A future message type beyond auth only needs a new branch inside the
// handler this is constructed with; this class itself never changes.
class WebSocketServer
{
public:
    using RequestHandler = std::function<std::string(const std::string& requestJson)>;

    WebSocketServer(int port, RequestHandler handler);
    ~WebSocketServer();

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    // Binds the port and starts accepting connections in the background.
    // Throws std::runtime_error if the port can't be bound.
    void start();

    // Blocks the calling thread until stop() is called.
    void wait();

    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
