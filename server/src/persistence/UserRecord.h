#ifndef USER_RECORD_H
#define USER_RECORD_H

#include <string>

// Plain row shape for the `users` table. Deliberately not named `User` -- a later
// networking step introduces a richer domain object (id + username + live session),
// and keeping the DB record separate now avoids a rename/refactor later.
struct UserRecord
{
    int id;
    std::string username;
    std::string password;
    int score;
};

#endif
