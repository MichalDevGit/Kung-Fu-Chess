#include "RepositoryFactory.h"

#include <stdexcept>

#include "InMemoryUserRepository.h"
#include "SqliteUserRepository.h"
#ifdef KUNGFUCHESS_HAS_POSTGRES
#include "PostgresUserRepository.h"
#endif

std::unique_ptr<IUserRepository> RepositoryFactory::createUserRepository(
    RepositoryBackend backend, const std::string& sqliteDbPath, const std::string& postgresConnectionString)
{
    switch (backend)
    {
    case RepositoryBackend::Sqlite:
        return std::make_unique<SqliteUserRepository>(sqliteDbPath);
    case RepositoryBackend::InMemory:
        return std::make_unique<InMemoryUserRepository>();
    case RepositoryBackend::Postgres:
#ifdef KUNGFUCHESS_HAS_POSTGRES
        return std::make_unique<PostgresUserRepository>(postgresConnectionString);
#else
        throw std::invalid_argument(
            "RepositoryFactory::createUserRepository: Postgres backend was not compiled in (libpq not found)");
#endif
    }

    throw std::invalid_argument("RepositoryFactory::createUserRepository: unknown backend");
}
