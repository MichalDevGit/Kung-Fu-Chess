// Standalone CLI client: a text-based substitute for the GUI client during
// early development. Connects to KungFuChessServer over plain ws:// and lets
// the user register/log in from an interactive terminal.
#include <iostream>
#include <mutex>
#include <string>

#include "cli/CliShell.h"
#include "network/WebSocketClient.h"

int main()
{
    const std::string url = "ws://127.0.0.1:9002";
    std::mutex outputMutex;

    WebSocketClient client(url, [&outputMutex](const std::string& responseJson)
                            {
                                std::lock_guard<std::mutex> lock(outputMutex);
                                std::cout << "\n[server] " << responseJson << "\n> " << std::flush;
                            });
    std::cout << "Connecting to " << url << " ...\n";
    client.start();

    if (!client.waitForConnection(5000))
    {
        std::cout << "Failed to connect to " << url << " within 5s -- is the server running?\n";
        return 1;
    }
    std::cout << "Connected.\n";

    CliShell shell(client, outputMutex);
    shell.run();

    return 0;
}
