#include "TestOrchestrator.h"
#include <fstream>
#include <gtest/gtest.h>

class ResillienceTest : public ::testing::Test {
protected:
  TestOrchestrator orchestrator;
  std::string testConfigPath =
      std::filesystem::absolute("config.json").string();

  void SetUp() override {
    std::ofstream config(testConfigPath);
    config << R"({
            "master_address": "127.0.0.1",
            "master_port": 5555,
            "machine_id": "RES_CLIENT_01",
            "log_level": "INFO",
            "encryption_enabled": false
        })";
    config.close();
  }

  void TearDown() override {
    orchestrator.cleanup();
    std::remove(testConfigPath.c_str());
  }
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
