#ifndef AUTH_REQUEST_HANDLER_H
#define AUTH_REQUEST_HANDLER_H

#include <string>

class AuthService;

// Translates a raw JSON request string into an AuthService call and a raw
// JSON response string. Knows the protocol/ message shapes; knows nothing
// about ixwebsocket or sockets, so it's usable and testable without a real
// connection -- WebSocketServer is the only thing that calls it.
class AuthRequestHandler
{
public:
    explicit AuthRequestHandler(AuthService& authService);

    // Never throws -- any parse/validation failure is caught internally and
    // turned into a protocol::ErrorResult envelope instead.
    std::string handle(const std::string& rawJson) const;

private:
    AuthService& authService;
};

#endif
