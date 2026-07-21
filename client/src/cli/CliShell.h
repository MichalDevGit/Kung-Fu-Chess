#ifndef CLI_SHELL_H
#define CLI_SHELL_H

#include <mutex>

class WebSocketClient;

// The interactive std::cin loop: parses "register <user> <pass>" /
// "login <user> <pass>" / "help" / "quit" and sends the matching request
// through the given WebSocketClient. Runs on the calling (main) thread.
// outputMutex is also locked by the caller's WebSocketClient message
// callback before it prints server responses, so prompts/usage text here
// and async server output there never interleave on the same terminal.
class CliShell
{
public:
    CliShell(WebSocketClient& client, std::mutex& outputMutex);

    // Blocks reading stdin until the user types "quit"/"exit" or EOF.
    void run();

private:
    WebSocketClient& client;
    std::mutex& outputMutex;
};

#endif
