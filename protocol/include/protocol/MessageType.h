#ifndef PROTOCOL_MESSAGE_TYPE_H
#define PROTOCOL_MESSAGE_TYPE_H

namespace protocol
{
    // Wire values for the JSON envelope's "type" field. Shared by server and
    // client so both sides dispatch on the same literal strings.
    namespace MessageType
    {
        inline constexpr const char* Register = "register";
        inline constexpr const char* Login = "login";
        inline constexpr const char* RegisterResult = "register_result";
        inline constexpr const char* LoginResult = "login_result";
        inline constexpr const char* Error = "error";
    }
}

#endif
