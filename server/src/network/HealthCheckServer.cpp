#include "network/HealthCheckServer.h"

#include <stdexcept>
#include <utility>

#include <ixwebsocket/IXHttpServer.h>
#include <ixwebsocket/IXNetSystem.h>

#include "common/Logging/Logger.h"

struct HealthCheckServer::Impl
{
    ix::HttpServer server;

    Impl(int port, const std::string& host)
        : server(port, host)
    {
    }
};

HealthCheckServer::HealthCheckServer(int port, std::string host)
    : impl(std::make_unique<Impl>(port, host))
{
    // Paired with uninitNetSystem() in the destructor -- harmless to call
    // alongside WebSocketServer's own init/uninit pair (see its .cpp), since
    // ix::initNetSystem/uninitNetSystem are reference-counted (WSAStartup/
    // WSACleanup on Windows), not a global on/off switch.
    ix::initNetSystem();

    impl->server.setOnConnectionCallback(
        [](ix::HttpRequestPtr, std::shared_ptr<ix::ConnectionState>) -> ix::HttpResponsePtr
        {
            return std::make_shared<ix::HttpResponse>(
                200,
                "OK",
                ix::HttpErrorCode::Ok,
                ix::WebSocketHttpHeaders{{"Content-Type", "application/json"}},
                "{\"status\":\"ok\"}");
        });
}

HealthCheckServer::~HealthCheckServer()
{
    stop();
    ix::uninitNetSystem();
}

void HealthCheckServer::start()
{
    const auto result = impl->server.listen();
    if (!result.first)
    {
        common::Logger::error("HealthCheckServer failed to listen: " + result.second);
        throw std::runtime_error("HealthCheckServer failed to listen: " + result.second);
    }

    impl->server.start();
}

void HealthCheckServer::stop()
{
    impl->server.stop();
}
