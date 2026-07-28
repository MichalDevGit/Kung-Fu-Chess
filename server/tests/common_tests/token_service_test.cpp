
#include "tests/doctest.h"
#include "common/Security/TokenService.h"
#include "common/Config/TokenConfig.h"

TEST_CASE("Testing TokenService")
{
    security::TokenService tokenService("test-secret");
    const long long now = 1'000'000;

    SUBCASE("issue then verify round-trips the userId")
    {
        const std::string token = tokenService.issue(42, now);
        const security::VerifyResult result = tokenService.verify(token, now + 1000);

        CHECK(result.valid);
        CHECK(result.userId == 42);
    }

    SUBCASE("a token verified after its TTL has elapsed is rejected as expired")
    {
        const std::string token = tokenService.issue(42, now);
        const security::VerifyResult result = tokenService.verify(token, now + TokenConfig::TOKEN_TTL_MILLIS + 1);

        CHECK_FALSE(result.valid);
        CHECK(result.error == "expired");
    }

    SUBCASE("a token signed with a different secret is rejected")
    {
        security::TokenService otherService("different-secret");
        const std::string token = otherService.issue(42, now);
        const security::VerifyResult result = tokenService.verify(token, now);

        CHECK_FALSE(result.valid);
        CHECK(result.error == "bad_signature");
    }

    SUBCASE("a tampered payload is rejected")
    {
        std::string token = tokenService.issue(42, now);
        const size_t dot = token.find('.');
        REQUIRE(dot != std::string::npos);
        token[0] = (token[0] == 'A') ? 'B' : 'A';

        const security::VerifyResult result = tokenService.verify(token, now);

        CHECK_FALSE(result.valid);
        CHECK(result.error == "bad_signature");
    }

    SUBCASE("a malformed token with no separator is rejected")
    {
        const security::VerifyResult result = tokenService.verify("not-a-token", now);

        CHECK_FALSE(result.valid);
        CHECK(result.error == "malformed");
    }
}
