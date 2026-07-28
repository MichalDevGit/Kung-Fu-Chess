#include "network/ApiGatewayClient.h"

#include <utility>

#include <ixwebsocket/IXHttpClient.h>
#include <nlohmann/json.hpp>

ApiGatewayClient::ApiGatewayClient(std::string baseUrl) : baseUrl(std::move(baseUrl))
{
}

protocol::RegisterResult ApiGatewayClient::registerUser(const std::string& username, const std::string& password) const
{
    ix::HttpClient httpClient;
    ix::HttpRequestArgsPtr args = httpClient.createRequest(baseUrl + "/register", ix::HttpClient::kPost);
    args->extraHeaders["Content-Type"] = "application/json";

    const ix::HttpResponsePtr response = httpClient.post(baseUrl + "/register", protocol::RegisterRequest{username, password}.toJson(), args);

    if (!response || response->errorCode != ix::HttpErrorCode::Ok)
        return protocol::RegisterResult{false, 0, "network_error"};

    try
    {
        return protocol::RegisterResult::fromJson(nlohmann::json::parse(response->body));
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::RegisterResult{false, 0, "malformed_response"};
    }
}

protocol::LoginResult ApiGatewayClient::login(const std::string& username, const std::string& password) const
{
    ix::HttpClient httpClient;
    ix::HttpRequestArgsPtr args = httpClient.createRequest(baseUrl + "/login", ix::HttpClient::kPost);
    args->extraHeaders["Content-Type"] = "application/json";

    const ix::HttpResponsePtr response = httpClient.post(baseUrl + "/login", protocol::LoginRequest{username, password}.toJson(), args);

    if (!response || response->errorCode != ix::HttpErrorCode::Ok)
        return protocol::LoginResult{false, 0, 0, "network_error", ""};

    try
    {
        return protocol::LoginResult::fromJson(nlohmann::json::parse(response->body));
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::LoginResult{false, 0, 0, "malformed_response", ""};
    }
}
