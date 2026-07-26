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
// handler passed to setRequestHandler(); this class itself never changes.
class WebSocketServer
{
public:
    using RequestHandler = std::function<std::string(const std::string& requestJson)>;

    explicit WebSocketServer(int port);
    ~WebSocketServer();

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    // Handler is set separately from construction (rather than passed to the
    // constructor) so main.cpp can construct this server first and capture a
    // reference to it in GameSessionManager's broadcast callback, before the
    // request handler (which depends on GameSessionManager existing) is
    // ready -- same deferred-registration problem WebSocketClient::setOnMessage
    // already solves on the client side, for the symmetric reason.
    void setRequestHandler(RequestHandler handler);

    // Sends json to every currently-connected client, unsolicited -- this is
    // what lets a server-driven change (a tick advancing the real-time clock,
    // a motion settling, a capture) actually reach a client instead of only
    // ever arriving as the reply to that same client's own request.
    void broadcast(const std::string& json);

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
