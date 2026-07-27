#ifndef PASSWORD_HASHER_H
#define PASSWORD_HASHER_H

#include <string>

// The only class in this codebase that knows a hashing algorithm exists.
// Wraps vendored bcrypt (see server/CMakeLists.txt's bcrypt_vendor target)
// behind two methods -- callers never see salt generation, cost factor, or
// the bcrypt hash string format itself.
class PasswordHasher
{
public:
    // Returns a self-contained hash string (bcrypt encodes its own salt and
    // cost factor into it) -- this is exactly what gets stored in the
    // `users.password` column.
    std::string hash(const std::string& plaintext) const;

    // True if plaintext, run through the same hashing the stored hash was
    // created with, matches it.
    bool verify(const std::string& plaintext, const std::string& storedHash) const;
};

#endif
