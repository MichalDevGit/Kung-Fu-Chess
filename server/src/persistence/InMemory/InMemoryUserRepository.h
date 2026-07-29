#ifndef IN_MEMORY_USER_REPOSITORY_H
#define IN_MEMORY_USER_REPOSITORY_H

#include <mutex>
#include <unordered_map>

#include "../IUserRepository.h"

// Dependency-free IUserRepository backed by a plain in-memory map -- no
// SQLite, no file/network I/O. Exists both as a fast test double and as
// proof the IUserRepository abstraction is real (not just interface
// theater): every method here honors the exact same contract
// SqliteUserRepository does, including throwing DuplicateUsernameException
// on a duplicate username. Guards its map with its own mutex, since (unlike
// SqliteUserRepository, which relies on SQLite's own connection handling)
// nothing else provides thread-safety for this container.
class InMemoryUserRepository : public IUserRepository
{
public:
    UserRecord createUser(const std::string& username, const std::string& passwordHash) override;
    std::optional<UserRecord> findByUsername(const std::string& username) const override;
    std::optional<UserRecord> findById(int id) const override;
    void setScore(int userId, int newScore) override;

private:
    mutable std::mutex mutex;
    std::unordered_map<int, UserRecord> usersById;
    int nextId = 1;
};

#endif
