#ifndef SQLITE_DATABASE_H
#define SQLITE_DATABASE_H

#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

// Owns the SQLite connection and creates the schema on construction.
// Pass a real file path for normal use, or ":memory:" for a disposable,
// per-test database.
class SqliteDatabase
{
public:
    explicit SqliteDatabase(const std::string& path);

    SQLite::Database& raw();

private:
    SQLite::Database db;

    void createSchema();
};

#endif
