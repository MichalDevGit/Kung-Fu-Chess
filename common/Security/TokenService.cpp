#include "common/Security/TokenService.h"

#include <algorithm>
#include <cstdint>

#include <nlohmann/json.hpp>

#include "common/Config/TokenConfig.h"
#include "common/Security/Sha256.h"

namespace
{
    const char* const BASE64URL_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    // Unpadded base64url -- fine here since this codec is only ever used to
    // round-trip through this class's own issue()/verify() pair, never
    // compared against an external base64 producer that might expect padding.
    std::string base64UrlEncode(const std::string& input)
    {
        std::string out;
        out.reserve(((input.size() + 2) / 3) * 4);

        size_t i = 0;
        while (i + 3 <= input.size())
        {
            const uint32_t chunk = (static_cast<uint8_t>(input[i]) << 16) | (static_cast<uint8_t>(input[i + 1]) << 8) |
                                   static_cast<uint8_t>(input[i + 2]);
            out.push_back(BASE64URL_ALPHABET[(chunk >> 18) & 0x3F]);
            out.push_back(BASE64URL_ALPHABET[(chunk >> 12) & 0x3F]);
            out.push_back(BASE64URL_ALPHABET[(chunk >> 6) & 0x3F]);
            out.push_back(BASE64URL_ALPHABET[chunk & 0x3F]);
            i += 3;
        }

        const size_t remaining = input.size() - i;
        if (remaining == 1)
        {
            const uint32_t chunk = static_cast<uint8_t>(input[i]) << 16;
            out.push_back(BASE64URL_ALPHABET[(chunk >> 18) & 0x3F]);
            out.push_back(BASE64URL_ALPHABET[(chunk >> 12) & 0x3F]);
        }
        else if (remaining == 2)
        {
            const uint32_t chunk = (static_cast<uint8_t>(input[i]) << 16) | (static_cast<uint8_t>(input[i + 1]) << 8);
            out.push_back(BASE64URL_ALPHABET[(chunk >> 18) & 0x3F]);
            out.push_back(BASE64URL_ALPHABET[(chunk >> 12) & 0x3F]);
            out.push_back(BASE64URL_ALPHABET[(chunk >> 6) & 0x3F]);
        }

        return out;
    }

    // Returns false on any invalid character -- callers treat that exactly
    // like every other malformed-token case.
    bool base64UrlDecode(const std::string& input, std::string& out)
    {
        int8_t lookup[256];
        std::fill(std::begin(lookup), std::end(lookup), -1);
        for (int i = 0; i < 64; ++i)
            lookup[static_cast<uint8_t>(BASE64URL_ALPHABET[i])] = static_cast<int8_t>(i);

        out.clear();
        uint32_t buffer = 0;
        int bitsCollected = 0;

        for (const char c : input)
        {
            const int8_t value = lookup[static_cast<uint8_t>(c)];
            if (value < 0)
                return false;

            buffer = (buffer << 6) | static_cast<uint32_t>(value);
            bitsCollected += 6;
            if (bitsCollected >= 8)
            {
                bitsCollected -= 8;
                out.push_back(static_cast<char>((buffer >> bitsCollected) & 0xFF));
            }
        }

        return true;
    }

    bool constantTimeEquals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
            return false;

        unsigned char diff = 0;
        for (size_t i = 0; i < a.size(); ++i)
            diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
        return diff == 0;
    }

}

namespace security
{
    TokenService::TokenService(std::string secret) : secret(std::move(secret))
    {
    }

    std::string TokenService::issue(int userId, long long nowMillis) const
    {
        const nlohmann::json payload{{"userId", userId}, {"exp", nowMillis + TokenConfig::TOKEN_TTL_MILLIS}};
        const std::string encodedPayload = base64UrlEncode(payload.dump());
        const Sha256Digest signature = hmacSha256(secret, encodedPayload);
        const std::string signatureBytes(signature.begin(), signature.end());
        return encodedPayload + "." + base64UrlEncode(signatureBytes);
    }

    VerifyResult TokenService::verify(const std::string& token, long long nowMillis) const
    {
        const size_t dot = token.find('.');
        if (dot == std::string::npos)
            return VerifyResult{false, 0, "malformed"};

        const std::string encodedPayload = token.substr(0, dot);
        const std::string encodedSignature = token.substr(dot + 1);

        const Sha256Digest expected = hmacSha256(secret, encodedPayload);
        const std::string expectedSignature(expected.begin(), expected.end());

        std::string actualSignature;
        if (!base64UrlDecode(encodedSignature, actualSignature) ||
            !constantTimeEquals(actualSignature, expectedSignature))
            return VerifyResult{false, 0, "bad_signature"};

        std::string decodedPayload;
        if (!base64UrlDecode(encodedPayload, decodedPayload))
            return VerifyResult{false, 0, "malformed"};

        try
        {
            const nlohmann::json payload = nlohmann::json::parse(decodedPayload);
            const long long exp = payload.at("exp").get<long long>();
            const int userId = payload.at("userId").get<int>();

            if (nowMillis >= exp)
                return VerifyResult{false, 0, "expired"};

            return VerifyResult{true, userId, ""};
        }
        catch (const nlohmann::json::exception&)
        {
            return VerifyResult{false, 0, "malformed"};
        }
    }
}
