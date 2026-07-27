#ifndef USER_REPOSITORY_EXCEPTIONS_H
#define USER_REPOSITORY_EXCEPTIONS_H

#include <stdexcept>
#include <string>

// Thrown by IUserRepository::createUser when the requested username already
// exists. Backend-agnostic on purpose -- callers (AuthService) must not need
// to know which concrete repository (SQLite, in-memory, ...) is behind the
// interface to detect this case.
class DuplicateUsernameException : public std::runtime_error
{
public:
    explicit DuplicateUsernameException(const std::string& username)
        : std::runtime_error("username already exists: " + username)
    {
    }
};

#endif
