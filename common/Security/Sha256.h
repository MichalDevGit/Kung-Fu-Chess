#ifndef COMMON_SECURITY_SHA256_H
#define COMMON_SECURITY_SHA256_H

#include <array>
#include <cstdint>
#include <string>

// Small, dependency-free SHA-256/HMAC-SHA256 implementation, vendored as
// plain source rather than pulled in via FetchContent -- same spirit as
// bcrypt_vendor (server/CMakeLists.txt) and doctest.h being vendored
// directly instead of adding a build-system dependency, except this one is
// simple enough to just write out rather than fetch. Used by
// common/Security/TokenService for signing/verifying login tokens.
namespace security
{
    using Sha256Digest = std::array<uint8_t, 32>;

    Sha256Digest sha256(const std::string& data);
    Sha256Digest hmacSha256(const std::string& key, const std::string& message);
}

#endif
