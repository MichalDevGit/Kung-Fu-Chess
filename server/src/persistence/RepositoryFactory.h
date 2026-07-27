#ifndef REPOSITORY_FACTORY_H
#define REPOSITORY_FACTORY_H

#include <memory>
#include <string>

#include "IUserRepository.h"

// Every user-persistence backend this codebase knows how to construct.
// Adding a new backend means one new value here plus one new case in
// RepositoryFactory::createUserRepository -- no call site elsewhere changes.
enum class RepositoryBackend
{
    Sqlite,
    InMemory
};

// The single place that decides which concrete IUserRepository gets built.
// Callers (server/main.cpp, tests) depend only on IUserRepository from here
// on -- swapping backends is a one-argument change at the call site, not a
// search-and-replace across the codebase.
class RepositoryFactory
{
public:
    // sqliteDbPath is only used (and required) when backend == Sqlite.
    static std::unique_ptr<IUserRepository> createUserRepository(
        RepositoryBackend backend, const std::string& sqliteDbPath = "");
};

#endif
