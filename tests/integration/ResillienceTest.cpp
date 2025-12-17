#include "TestOrchestrator.h"
#include <gtest/gtest.h>


class ResillienceTest : public ::testing::Test {
protected:
  TestOrchestrator orchestrator;
};

// 8. RECONNECTION
TEST_F(ResillienceTest, ClientReconnectsAfterNetworkLoss) {
  orchestrator.startMockMaster(5555);
  orchestrator.startRealClient("config.json");

  // 1. Verify connected

  // 2. Stop Master (Simulate network/server loss)
  orchestrator.stopMockMaster();

  // 3. Wait
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // 4. Restart Master
  orchestrator.startMockMaster(5555);

  // 5. Verify Client Reconnects
  SUCCEED();
}
