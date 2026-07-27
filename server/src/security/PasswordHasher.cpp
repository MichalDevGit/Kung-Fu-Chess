#include "PasswordHasher.h"

#include "bcrypt/BCrypt.hpp"

#include "SecurityConfig.h"

std::string PasswordHasher::hash(const std::string& plaintext) const
{
    return BCrypt::generateHash(plaintext, SecurityConfig::BCRYPT_COST_FACTOR);
}

bool PasswordHasher::verify(const std::string& plaintext, const std::string& storedHash) const
{
    return BCrypt::validatePassword(plaintext, storedHash);
}
