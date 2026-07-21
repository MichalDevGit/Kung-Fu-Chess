#include "Database.h"

Database::Database(const std::string& path)
    : db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    createSchema();
}

SQLite::Database& Database::raw()
{
    return db;
}

void Database::createSchema()
{
    db.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id         INTEGER PRIMARY KEY,"
        "  username   TEXT UNIQUE NOT NULL,"
        "  password   TEXT NOT NULL,"
        "  score      INTEGER NOT NULL DEFAULT 0,"
        "  created_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ")");
}
