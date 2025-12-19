#include "cms/Socket.h"
#include <gtest/gtest.h>


// Forward declarations of functions in Network.cpp
namespace cms {
namespace network {
void InitializeNetwork();
void ShutdownNetwork();
} // namespace network
} // namespace cms

TEST(NetworkTest, InitializationAndCleanup) {
  // Just verify that these functions can be called without crashing
  // and that they return (void).
  // The actual effect (WSAStartup) is global and hard to verify in isolation
  // without side effects, but Socket::Initialize returns bool which we ignore
  // here based on the void signature in Network.cpp.

  EXPECT_NO_THROW({ cms::network::InitializeNetwork(); });

  EXPECT_NO_THROW({ cms::network::ShutdownNetwork(); });
}
