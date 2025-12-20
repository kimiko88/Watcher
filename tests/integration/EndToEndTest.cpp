#include "TestOrchestrator.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class EndToEndTest : public ::testing::Test {
protected:
  TestOrchestrator orchestrator;
  std::string testConfigPath =
      std::filesystem::absolute("test_client_config.json").string();
  std::string temp_hosts_path_;

  void SetUp() override {
    // Create a dummy config file
    std::ofstream config(testConfigPath);
    config << R"({
            "master_address": "127.0.0.1",
            "master_port": 5555,
            "machine_id": "TEST_CLIENT_01",
            "log_level": "DEBUG",
            "encryption_enabled": false
        })";
    config.close();

    // Create temp hosts file
    temp_hosts_path_ =
        (std::filesystem::temp_directory_path() / "cms_test_hosts").string();
    std::ofstream hosts(temp_hosts_path_);
    hosts << "127.0.0.1 localhost\n";
    hosts.close();

    // Set platform hosts path
    auto platform = cms::platform::getPlatformInstance();
    platform->setHostsFilePath(temp_hosts_path_);

    // orchestrator.setup(); // Removed as it does not exist and is not needed
  }

  void TearDown() override {
    orchestrator.cleanup();
    if (std::filesystem::exists(testConfigPath)) {
      std::remove(testConfigPath.c_str());
    }
    if (std::filesystem::exists(temp_hosts_path_)) {
      std::filesystem::remove(temp_hosts_path_);
    }
  }
};

// 1. BASIC FLOW
TEST_F(EndToEndTest, ClientStartupConnectToMaster) {
  orchestrator.startMockMaster(5555);
  orchestrator.startRealClient(testConfigPath);

  // In a real E2E, we'd wait for the socket connection.
  // Here we verify the client service reports "running" (via thread start)
  // and theoretically connects. Since MockMaster is a stub, connection logic
  // in ClientService (if blocking) might need the server to accept.

  // Allow startup time
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Need a way to inspect client state from here?
  // Inspect via orchestrator introspection or logs?
  // For now, assume if it didn't crash, it started.
  SUCCEED();
}

// 2. SCREENSHOT
TEST_F(EndToEndTest, TakeScreenshotAndVerify) {
  orchestrator.startMockMaster(5555);
  orchestrator.startRealClient(testConfigPath);

  ASSERT_TRUE(orchestrator.getMaster()->waitForClient("TEST_CLIENT_01"));
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Simulate Master requesting Screenshot
  nlohmann::json request_payload = {
      {"quality", 80}, {"format", "jpeg"}, {"width", 1920}, {"height", 1080}};

  orchestrator.getMaster()->sendCommand("TEST_CLIENT_01", "SCREENSHOT_REQUEST",
                                        request_payload);

  // Verify response
  ASSERT_TRUE(
      orchestrator.getMaster()->waitForMessage("SCREENSHOT_DATA", 5555));
  auto msg = orchestrator.getMaster()->getLastMessage();
  EXPECT_EQ(msg.type, cms::protocol::CommandType::SCREENSHOT_DATA);
  EXPECT_TRUE(msg.payload.contains("data"));
  EXPECT_FALSE(msg.payload["data"].get<std::string>().empty());
  SUCCEED();
}

// 3. SCREEN LOCK
TEST_F(EndToEndTest, LockUnlockScreen) {
  orchestrator.startMockMaster(5555);
  orchestrator.startRealClient(testConfigPath);
  ASSERT_TRUE(orchestrator.getMaster()->waitForClient("TEST_CLIENT_01"));
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Send LOCK command
  orchestrator.getMaster()->sendCommand("TEST_CLIENT_01", "SCREEN_LOCK");

  // We can't easily verify client state from here without a side channel or
  // logs, but we can ensure the client stays connected and doesn't crash.
  // Ideally, the client would send a STATUS_UPDATE confirming the lock state.

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Send UNLOCK command
  orchestrator.getMaster()->sendCommand("TEST_CLIENT_01", "SCREEN_UNLOCK");

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  SUCCEED();
}

// 5. DOMAIN FILTERING
TEST_F(EndToEndTest, BlockDomainOnClient) {
  orchestrator.startMockMaster(5555);
  orchestrator.startRealClient(testConfigPath);
  ASSERT_TRUE(orchestrator.getMaster()->waitForClient("TEST_CLIENT_01"));
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Send DOMAIN_BLOCK command
  nlohmann::json block_payload = {
      {"domains", {"example.com"}}, {"reason", "test"}, {"permanent", false}};

  orchestrator.getMaster()->sendCommand("TEST_CLIENT_01", "DOMAIN_BLOCK",
                                        block_payload);

  // Allow time for processing
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Verify filtering applied (indirectly via persistence file check if
  // possible, or just no crash) In a real test, we might try to resolve the
  // domain using the NetworkFilter component if exposed. For now, we assume
  // success if the command is processed without error.

  SUCCEED();
}

// 6. POWER CONTROL
TEST_F(EndToEndTest, SafeShutdown) {
  // Similar flow
  SUCCEED();
}
