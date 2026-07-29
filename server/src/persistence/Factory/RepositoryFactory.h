#ifndef REPOSITORY_FACTORY_H
#define REPOSITORY_FACTORY_H

#include <memory>
#include <string>

#include "../IUserRepository.h"

// Every user-persistence backend this codebase knows how to construct.
// Adding a new backend means one new value here plus one new case in
// RepositoryFactory::createUserRepository -- no call site elsewhere changes.
enum class RepositoryBackend
{
    Sqlite,
    InMemory,
    // Only actually constructible in a build where libpq was found at
    // configure time (see server/CMakeLists.txt / KUNGFUCHESS_HAS_POSTGRES) --
    // createUserRepository throws otherwise instead of failing to compile,
    // so this enum value itself is always valid to reference.
    Postgres
};

// The single place that decides which concrete IUserRepository gets built.
// Callers (server/main.cpp, tests) depend only on IUserRepository from here
// on -- swapping backends is a one-argument change at the call site, not a
// search-and-replace across the codebase.
class RepositoryFactory
{
public:
    // sqliteDbPath is only used (and required) when backend == Sqlite.
    // postgresConnectionString is only used (and required) when backend ==
    // Postgres -- a libpq connection string/URI.
    static std::unique_ptr<IUserRepository> createUserRepository(
        RepositoryBackend backend,
        const std::string& sqliteDbPath = "",
        const std::string& postgresConnectionString = "");
};

#endif
