#include "tests/doctest.h"
#include "services/ConnectionRegistry.h"
#include "services/LocalConnectionStore.h"

namespace
{
ConnectionRegistry::AuthenticatedUser alice()
{
    return ConnectionRegistry::AuthenticatedUser{1, "alice", 1200};
}
}

TEST_CASE("ConnectionRegistry(LocalConnectionStore) binds and finds a connection both directions")
{
    LocalConnectionStore store;
    ConnectionRegistry registry(store);

    registry.onAuthenticated("conn-1", alice());

    const std::optional<ConnectionRegistry::AuthenticatedUser> found = registry.find("conn-1");
    REQUIRE(found.has_value());
    CHECK(found->userId == 1);
    CHECK(found->username == "alice");

    const std::optional<std::string> connectionId = registry.findConnectionForUser(1);
    REQUIRE(connectionId.has_value());
    CHECK(*connectionId == "conn-1");
}

TEST_CASE("ConnectionRegistry::find/findConnectionForUser return nullopt when unknown")
{
    LocalConnectionStore store;
    ConnectionRegistry registry(store);

    CHECK_FALSE(registry.find("conn-nobody").has_value());
    CHECK_FALSE(registry.findConnectionForUser(999).has_value());
}

TEST_CASE("A user re-authenticating on a new connection supersedes the old one")
{
    LocalConnectionStore store;
    ConnectionRegistry registry(store);

    registry.onAuthenticated("conn-1", alice());
    registry.onAuthenticated("conn-2", alice());

    CHECK_FALSE(registry.find("conn-1").has_value()); // stale forward mapping dropped
    CHECK(registry.find("conn-2").has_value());
    CHECK(*registry.findConnectionForUser(1) == "conn-2");
}

TEST_CASE("onDisconnected for a stale, already-superseded connection doesn't touch the newer binding")
{
    LocalConnectionStore store;
    ConnectionRegistry registry(store);

    registry.onAuthenticated("conn-1", alice());
    registry.onAuthenticated("conn-2", alice()); // supersedes conn-1

    registry.onDisconnected("conn-1"); // a stale close arriving after the supersede

    CHECK(*registry.findConnectionForUser(1) == "conn-2");
    CHECK(registry.find("conn-2").has_value());
}

TEST_CASE("onDisconnected for the current connection erases both directions")
{
    LocalConnectionStore store;
    ConnectionRegistry registry(store);

    registry.onAuthenticated("conn-1", alice());
    registry.onDisconnected("conn-1");

    CHECK_FALSE(registry.find("conn-1").has_value());
    CHECK_FALSE(registry.findConnectionForUser(1).has_value());
}
