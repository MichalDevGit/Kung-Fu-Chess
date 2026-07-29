#ifndef POSTGRES_DATABASE_H
#define POSTGRES_DATABASE_H

// Whole file is a no-op when libpq wasn't found at configure time (see
// server/CMakeLists.txt) -- KUNGFUCHESS_HAS_POSTGRES is only ever defined
// when PostgresUserRepository is actually being compiled/linked, so no
// translation unit that includes this header needs libpqxx present otherwise.
#ifdef KUNGFUCHESS_HAS_POSTGRES

#include <string>

#include <pqxx/pqxx>

// Owns the Postgres connection and creates the schema on construction --
// same role Database plays for SqliteUserRepository. connectionString is a
// libpq connection string/URI (e.g. "postgresql://user:pass@host:5432/db").
class PostgresDatabase
{
public:
    explicit PostgresDatabase(const std::string& connectionString);

    pqxx::connection& raw();

private:
    pqxx::connection conn;

    void createSchema();
};

#endif // KUNGFUCHESS_HAS_POSTGRES
#endif // POSTGRES_DATABASE_H
