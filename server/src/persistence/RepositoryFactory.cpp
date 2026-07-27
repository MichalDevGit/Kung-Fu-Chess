#include "RepositoryFactory.h"

#include <stdexcept>

#include "InMemoryUserRepository.h"
#include "SqliteUserRepository.h"

std::unique_ptr<IUserRepository> RepositoryFactory::createUserRepository(
    RepositoryBackend backend, const std::string& sqliteDbPath)
{
    switch (backend)
    {
    case RepositoryBackend::Sqlite:
        return std::make_unique<SqliteUserRepository>(sqliteDbPath);
    case RepositoryBackend::InMemory:
        return std::make_unique<InMemoryUserRepository>();
    }

    throw std::invalid_argument("RepositoryFactory::createUserRepository: unknown backend");
}
