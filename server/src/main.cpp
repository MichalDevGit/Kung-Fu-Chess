// Standalone entry point for the persistence layer: creates/opens the SQLite
// database file on disk (so it can be inspected with a SQLite VS Code
// extension) and seeds a couple of demo users if the table is empty, then
// prints the current contents of `users` back out for a quick sanity check.
#include <filesystem>
#include <iostream>

#include "persistence/Database.h"
#include "persistence/UserRepository.h"

int main()
{
    const std::string dbPath = "kungfuchess.db";
    Database database(dbPath);
    UserRepository users(database);

    if (!users.findByUsername("alice").has_value())
        users.createUser("alice", "password123");
    if (!users.findByUsername("bob").has_value())
        users.createUser("bob", "hunter2");

    std::cout << "Database file: " << std::filesystem::absolute(dbPath) << "\n\n";
    std::cout << "id\tusername\tpassword\tscore\n";
    for (const std::string& name : {"alice", "bob"})
    {
        const auto user = users.findByUsername(name);
        if (!user.has_value())
            continue;

        std::cout << user->id << '\t' << user->username << '\t'
                   << user->password << '\t' << user->score << '\n';
    }

    return 0;
}
