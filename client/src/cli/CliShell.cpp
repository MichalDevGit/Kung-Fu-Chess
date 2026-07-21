#include "cli/CliShell.h"

#include <iostream>
#include <sstream>
#include <string>

#include "network/WebSocketClient.h"
#include "protocol/Message.h"

namespace
{
    void printHelp()
    {
        std::cout << "Commands:\n"
                     "  register <username> <password>\n"
                     "  login <username> <password>\n"
                     "  help\n"
                     "  quit\n";
    }
}

CliShell::CliShell(WebSocketClient& client, std::mutex& outputMutex)
    : client(client)
    , outputMutex(outputMutex)
{
}

void CliShell::run()
{
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        printHelp();
    }

    std::string line;
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "> " << std::flush;
        }

        if (!std::getline(std::cin, line))
            break;

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command.empty())
            continue;

        if (command == "quit" || command == "exit")
            break;

        if (command == "help")
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            printHelp();
            continue;
        }

        if (command == "register" || command == "login")
        {
            std::string username;
            std::string password;
            iss >> username >> password;
            if (username.empty() || password.empty())
            {
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "usage: " << command << " <username> <password>\n";
                continue;
            }

            const std::string requestJson = (command == "register")
                ? protocol::RegisterRequest{username, password}.toJson()
                : protocol::LoginRequest{username, password}.toJson();
            client.send(requestJson);
            continue;
        }

        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Unknown command '" << command << "' -- type 'help' for a list.\n";
    }
}
