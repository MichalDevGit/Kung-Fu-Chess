#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <mutex>
#include <string>

class UserRepository;

namespace auth
{
    struct RegisterOutcome
    {
        bool success;
        int userId;
        std::string error; // set to "username_taken" when !success
    };

    struct LoginOutcome
    {
        bool success;
        int userId;
        int score;
        std::string error; // set to "invalid_credentials" when !success
    };
}

// Thin, networking-agnostic wrapper around UserRepository -- the only new
// DB-touching logic added for the WebSocket layer. Guards every call with a
// mutex because (once wired into WebSocketServer) multiple client
// connections can call in concurrently, while UserRepository/Database share
// a single, non-synchronized SQLite connection.
class AuthService
{
public:
    explicit AuthService(UserRepository& users);

    auth::RegisterOutcome registerUser(const std::string& username, const std::string& password);
    auth::LoginOutcome login(const std::string& username, const std::string& password);

private:
    UserRepository& users;
    std::mutex mutex;
};

#endif
