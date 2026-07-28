#include "network/ApiGatewayServer.h"

#include <stdexcept>
#include <utility>

#include <ixwebsocket/IXHttpServer.h>
#include <ixwebsocket/IXNetSystem.h>

#include "ApiGatewayRequestHandler.h"
#include "common/Logging/Logger.h"

namespace
{
    ix::HttpResponsePtr jsonResponse(int statusCode, const std::string& body)
    {
        return std::make_shared<ix::HttpResponse>(
            statusCode,
            "OK",
            ix::HttpErrorCode::Ok,
            ix::WebSocketHttpHeaders{{"Content-Type", "application/json"}},
            body);
    }
}

struct ApiGatewayServer::Impl
{
    ix::HttpServer server;

    Impl(int port, const std::string& host)
        : server(port, host)
    {
    }
};

ApiGatewayServer::ApiGatewayServer(ApiGatewayRequestHandler& handler, int port, std::string host)
    : impl(std::make_unique<Impl>(port, host))
{
    // Paired with uninitNetSystem() in the destructor -- see
    // HealthCheckServer.cpp for why this is safe to call alongside the
    // WebSocket server's own init/uninit pair (reference-counted, not a
    // global on/off switch).
    ix::initNetSystem();

    impl->server.setOnConnectionCallback(
        [&handler](ix::HttpRequestPtr request, std::shared_ptr<ix::ConnectionState>) -> ix::HttpResponsePtr
        {
            if (request->method == "POST" && request->uri == "/register")
                return jsonResponse(200, handler.handleRegister(request->body));

            if (request->method == "POST" && request->uri == "/login")
                return jsonResponse(200, handler.handleLogin(request->body));

            return jsonResponse(404, "{\"error\":\"not_found\"}");
        });
}

ApiGatewayServer::~ApiGatewayServer()
{
    stop();
    ix::uninitNetSystem();
}

void ApiGatewayServer::start()
{
    const auto result = impl->server.listen();
    if (!result.first)
    {
        common::Logger::error("ApiGatewayServer failed to listen: " + result.second);
        throw std::runtime_error("ApiGatewayServer failed to listen: " + result.second);
    }

    impl->server.start();
}

void ApiGatewayServer::wait()
{
    impl->server.wait();
}

void ApiGatewayServer::stop()
{
    impl->server.stop();
}
