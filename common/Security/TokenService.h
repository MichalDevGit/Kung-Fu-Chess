#ifndef COMMON_SECURITY_TOKEN_SERVICE_H
#define COMMON_SECURITY_TOKEN_SERVICE_H

#include <string>

namespace security
{
    struct VerifyResult
    {
        bool valid = false;
        int userId = 0;
        std::string error; // set when !valid: "malformed"/"bad_signature"/"expired"
    };

    // Issues/verifies a small homegrown signed token used to let the
    // WebSocket process (server/) trust a userId without ever verifying a
    // password itself -- that verification happens once, in the API
    // Gateway process (apigateway/), which is the only thing that calls
    // issue(); the WebSocket process only ever calls verify(). Both
    // processes must be constructed with the same secret string (see
    // common/Config/TokenConfig.h).
    //
    // Deliberately NOT a JWT -- no "alg"/"typ" header section, just
    // base64url(payload_json) + "." + base64url(HMAC-SHA256(secret, payload)),
    // built on common/Security/Sha256.h. Nothing outside this codebase ever
    // needs to consume these tokens, so there's no interoperability reason
    // to pull in a real JWT library (and the heavier OpenSSL dependency
    // that would come with one -- see ARCHITECTURE.md's Build system notes
    // on why libpq-style dependencies are avoided where possible).
    class TokenService
    {
    public:
        explicit TokenService(std::string secret);

        // nowMillis must be real (Unix-epoch) wall-clock time -- see
        // common/WallClock.h's wallClockMillis(), NOT common/MonotonicClock.h's
        // nowMillis(), since issue() and verify() run in two different
        // processes that only agree on wall-clock time, not on
        // steady_clock's per-process-meaningful-only epoch. Passed in
        // rather than read internally so this class stays a pure,
        // easily-testable function of its inputs.
        std::string issue(int userId, long long nowMillis) const;
        VerifyResult verify(const std::string& token, long long nowMillis) const;

    private:
        std::string secret;
    };
}

#endif
