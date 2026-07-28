#include "PostgresDatabase.h"

#ifdef KUNGFUCHESS_HAS_POSTGRES

PostgresDatabase::PostgresDatabase(const std::string& connectionString)
    : conn(connectionString)
{
    createSchema();
}

pqxx::connection& PostgresDatabase::raw()
{
    return conn;
}

void PostgresDatabase::createSchema()
{
    // score's "DEFAULT 0" below is never actually relied on -- every
    // IUserRepository::createUser implementation always inserts an explicit
    // RatingConfig::INITIAL_RATING, same as SqliteUserRepository's schema.
    pqxx::work txn(conn);
    txn.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id         SERIAL PRIMARY KEY,"
        "  username   TEXT UNIQUE NOT NULL,"
        "  password   TEXT NOT NULL,"
        "  score      INTEGER NOT NULL DEFAULT 0,"
        "  created_at TIMESTAMPTZ DEFAULT now()"
        ")");
    txn.commit();
}

#endif // KUNGFUCHESS_HAS_POSTGRES
