#include "RemoteGameNodeHandler.h"

#include "GameNodeRequestPublisher.h"

RemoteGameNodeHandler::RemoteGameNodeHandler(GameNodeRequestPublisher& publisher) : publisher(publisher)
{
}

std::string RemoteGameNodeHandler::handle(const std::string& connectionId, const std::string& rawJson) const
{
    publisher.forward(connectionId, rawJson);
    return "{}";
}
