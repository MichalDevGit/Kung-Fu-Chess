#include "tests/doctest.h"

#include "security/PasswordHasher.h"

TEST_CASE("Testing PasswordHasher")
{
    PasswordHasher hasher;

    SUBCASE("hash never returns the plaintext back")
    {
        const std::string hashed = hasher.hash("hunter2");
        CHECK(hashed != "hunter2");
    }

    SUBCASE("hashing the same password twice produces different hashes (random salt per call)")
    {
        const std::string first = hasher.hash("hunter2");
        const std::string second = hasher.hash("hunter2");
        CHECK(first != second);
    }

    SUBCASE("verify succeeds for the correct password against its own hash")
    {
        const std::string hashed = hasher.hash("correct-password");
        CHECK(hasher.verify("correct-password", hashed));
    }

    SUBCASE("verify fails for the wrong password against a valid hash")
    {
        const std::string hashed = hasher.hash("correct-password");
        CHECK_FALSE(hasher.verify("wrong-password", hashed));
    }
}
