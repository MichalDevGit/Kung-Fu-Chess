#include "cli/CliShell.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "network/ApiGatewayClient.h"
#include "network/WebSocketClient.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"

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

    constexpr std::chrono::seconds LOGIN_TIMEOUT{5};
}

CliShell::CliShell(WebSocketClient& client, ApiGatewayClient& apiGatewayClient, std::mutex& outputMutex)
    : client(client)
    , apiGatewayClient(apiGatewayClient)
    , outputMutex(outputMutex)
{
}

CliShell::LoginOutcome CliShell::run()
{
    client.setOnMessage([this](const std::string& json)
        {
            onMessage(json);
        });

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

            // register/login are now REST calls against the API Gateway
            // (MIGRATION_PLAN.md Phase 2) -- register needs nothing further,
            // it never touched this WebSocket connection even before.
            if (command == "register")
            {
                const protocol::RegisterResult result = apiGatewayClient.registerUser(username, password);
                std::lock_guard<std::mutex> lock(outputMutex);
                if (result.success)
                    std::cout << "registered as user id " << result.userId << "\n";
                else
                    std::cout << "register failed: " << result.error << "\n";
                continue;
            }

            const protocol::LoginResult gatewayResult = apiGatewayClient.login(username, password);
            if (!gatewayResult.success)
            {
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "login failed: " << gatewayResult.error << "\n";
                continue;
            }

            // The REST call only proved the password was correct and handed
            // back a token -- this connection's identity isn't bound until
            // that token is presented over the WebSocket itself (see
            // AuthRequestHandler's login_token branch), so block for the
            // matching login_result exactly as the old password-based login
            // used to (see onMessage).
            std::unique_lock<std::mutex> resultLock(resultMutex);
            pendingLoginResult.reset();
            pendingMatchFound.reset();
            resultLock.unlock();

            client.send(protocol::LoginWithTokenRequest{gatewayResult.token}.toJson());

            resultLock.lock();
            const bool arrived = resultCv.wait_for(resultLock, LOGIN_TIMEOUT, [this]
                { return pendingLoginResult.has_value(); });

            if (arrived && pendingLoginResult->success)
            {
                return LoginOutcome{
                    true, pendingLoginResult->userId, username, pendingLoginResult->score, pendingMatchFound};
            }

            const std::string reason = arrived ? pendingLoginResult->error : std::string("timed out waiting for server");
            resultLock.unlock();

            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "login failed: " << reason << "\n";
            continue;
        }

        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Unknown command '" << command << "' -- type 'help' for a list.\n";
    }

    return LoginOutcome{};
}

void CliShell::onMessage(const std::string& json)
{
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "\n[server] " << json << "\n> " << std::flush;
    }

    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(json);
        const std::string type = protocol::readType(parsed);

        if (type == protocol::MessageType::LoginResult)
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            pendingLoginResult = protocol::LoginResult::fromJson(parsed);
            resultCv.notify_all();
        }
        else if (type == protocol::MessageType::MatchFound)
        {
            // A reconnect resume -- see LoginOutcome::resumedMatch. Always
            // arrives before login_result on the same connection (pushed
            // synchronously inside AuthRequestHandler::handle before it
            // returns the login_result reply), so this is guaranteed to
            // already be set by the time the wait below wakes up.
            std::lock_guard<std::mutex> lock(resultMutex);
            pendingMatchFound = protocol::MatchFoundResult::fromJson(parsed);
        }
    }
    catch (const nlohmann::json::exception&)
    {
        // Malformed message from the server -- same never-throw philosophy
        // as GameClient::onMessage; a bad frame shouldn't crash the shell.
    }
}
