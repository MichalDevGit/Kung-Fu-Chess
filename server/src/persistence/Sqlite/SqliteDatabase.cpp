#include "SqliteDatabase.h"

SqliteDatabase::SqliteDatabase(const std::string& path)
    : db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    createSchema();
}

SQLite::Database& SqliteDatabase::raw()
{
    return db;
}

void SqliteDatabase::createSchema()
{
    // score's "DEFAULT 0" below is never actually relied on -- every
    // IUserRepository::createUser implementation always inserts an explicit
    // RatingConfig::INITIAL_RATING, so C++ (not this schema) is the one
    // source of truth for the starting rating.
    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id         INTEGER PRIMARY KEY,"
        "  username   TEXT UNIQUE NOT NULL,"
        "  password   TEXT NOT NULL,"
        "  score      INTEGER NOT NULL DEFAULT 0,"
        "  created_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ")");
}
