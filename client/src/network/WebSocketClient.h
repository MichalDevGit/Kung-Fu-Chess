#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <functional>
#include <memory>
#include <string>

// The only header/source pair that includes ixwebsocket's client-side
// headers (see the .cpp). This is deliberately the exact class a future GUI
// client should link (via the KungFuChessClientNetwork target) instead of
// talking to ixwebsocket directly -- the wire-level connection logic is
// written once here.
class WebSocketClient
{
public:
    using MessageHandler = std::function<void(const std::string& responseJson)>;

    // url looks like "ws://127.0.0.1:9002". onMessage is invoked from a
    // background thread owned by ixwebsocket for every inbound message, so
    // callers touching shared state (e.g. std::cout) from it must synchronize.
    WebSocketClient(const std::string& url, MessageHandler onMessage);
    ~WebSocketClient();

    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;

    // Starts connecting in the background (and auto-reconnects on drop).
    void start();

    // Blocks until the connection is open, or timeoutMs elapses. Sending
    // before the handshake completes silently drops the message (ixwebsocket
    // only queues messages once the socket is open), so callers should wait
    // for this before sending anything.
    bool waitForConnection(int timeoutMs);

    void send(const std::string& requestJson);

    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
