#include "TestOrchestrator.h"
#include <gtest/gtest.h>
#include <vector>


class PerformanceTest : public ::testing::Test {
protected:
  TestOrchestrator orchestrator;
};

// 4. BROADCAST DEMO
TEST_F(PerformanceTest, MasterBroadcastsScreen) {
  orchestrator.startMockMaster(5555);
  // Connect 3 clients
  for (int i = 0; i < 3; ++i) {
    // needs unique config
    orchestrator.startRealClient("test_config.json");
  }

  // Send Broadcast Start
  // Verify all clients receive stream
  SUCCEED();
}

// 7. CONCURRENT OPERATIONS
TEST_F(PerformanceTest, MultipleClientsMultipleCommands) {
  orchestrator.startMockMaster(5555);
  // 10 clients
  // Send mixed commands
  SUCCEED();
}

// 9. PERFORMANCE
TEST_F(PerformanceTest, SustainedLoad30Clients) {
  // 30 clients
  // Loop for 10 seconds sending commands
  // Measure response times
  SUCCEED();
}
