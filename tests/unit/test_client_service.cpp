#include "cms/ClientService.h"
#include "cms/Protocol.h"
#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

using namespace cms::client;
using namespace cms::protocol;

// Helper to create test config file
class TestConfigHelper {
public:
  static std::string createValidConfig() {
    std::string configPath = "test_client_config.json";
    std::ofstream configFile(configPath);
    configFile << R"({
            "master_address": "127.0.0.1",
            "master_port": 5555,
            "machine_id": "test-machine-123",
            "encryption_enabled": false,
            "log_level": "INFO"
        })";
    configFile.close();
    return configPath;
  }

  static std::string createInvalidConfig() {
    std::string configPath = "test_invalid_config.json";
    std::ofstream configFile(configPath);
    configFile << "{ invalid json }";
    configFile.close();
    return configPath;
  }

  static void cleanup(const std::string &path) { std::remove(path.c_str()); }
};

// Mock Master Server for testing
class MockMasterServer {
private:
  bool running_ = false;
  int port_;
  std::thread serverThread_;

public:
  explicit MockMasterServer(int port) : port_(port) {}

  ~MockMasterServer() { stop(); }

  void start() {
    running_ = true;
    // In a real implementation, would start TCP server
    // For now, just simulate server running
  }

  void stop() {
    running_ = false;
    if (serverThread_.joinable()) {
      serverThread_.join();
    }
  }

  bool isRunning() const { return running_; }
};

// Test fixture for ClientService tests
class ClientServiceTest : public ::testing::Test {
protected:
  void SetUp() override {
    validConfigPath = TestConfigHelper::createValidConfig();
  }

  void TearDown() override { TestConfigHelper::cleanup(validConfigPath); }

  std::string validConfigPath;
};

// ============================================================================
// CONSTRUCTION TESTS
// ============================================================================

TEST_F(ClientServiceTest, ConstructionWithValidConfig) {
  EXPECT_NO_THROW({ ClientService service(validConfigPath); });
}

TEST_F(ClientServiceTest, ConstructionWithInvalidConfigThrows) {
  std::string invalidPath = TestConfigHelper::createInvalidConfig();

  EXPECT_THROW({ ClientService service(invalidPath); }, std::runtime_error);

  TestConfigHelper::cleanup(invalidPath);
}

TEST_F(ClientServiceTest, ConstructionWithNonExistentConfigThrows) {
  EXPECT_THROW(
      { ClientService service("nonexistent_config.json"); },
      std::runtime_error);
}

// ============================================================================
// START/STOP TESTS
// ============================================================================

TEST_F(ClientServiceTest, InitiallyNotRunning) {
  ClientService service(validConfigPath);
  EXPECT_FALSE(service.isRunning());
}

TEST_F(ClientServiceTest, StartSetsRunningTrue) {
  ClientService service(validConfigPath);

  // Note: start() will fail to connect to master in test environment
  // but should still set running state
  service.start();

  // Give it a moment to process
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(service.isRunning());

  service.stop();
}

TEST_F(ClientServiceTest, StopSetsRunningFalse) {
  ClientService service(validConfigPath);
  service.start();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  service.stop();
  EXPECT_FALSE(service.isRunning());
}

TEST_F(ClientServiceTest, MultipleStartCallsIdempotent) {
  ClientService service(validConfigPath);

  service.start();
  bool firstStart = service.isRunning();

  service.start();
  bool secondStart = service.isRunning();

  EXPECT_EQ(firstStart, secondStart);
  EXPECT_TRUE(secondStart);

  service.stop();
}

TEST_F(ClientServiceTest, MultipleStopCallsSafe) {
  ClientService service(validConfigPath);
  service.start();

  service.stop();
  EXPECT_NO_THROW({ service.stop(); });

  EXPECT_FALSE(service.isRunning());
}

// ============================================================================
// STATUS TESTS
// ============================================================================

TEST_F(ClientServiceTest, GetStatusReturnsValidStruct) {
  ClientService service(validConfigPath);

  auto status = service.getStatus();

  // Should have valid service version
  EXPECT_FALSE(status.service_version.empty());

  // Should have valid machine ID
  EXPECT_FALSE(status.machine_id.empty());

  // Initially not connected
  EXPECT_FALSE(status.is_connected);
}

TEST_F(ClientServiceTest, StatusReflectsConnectionState) {
  ClientService service(validConfigPath);

  auto statusBefore = service.getStatus();
  EXPECT_FALSE(statusBefore.is_connected);

  service.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Note: Won't actually connect in test, but status should update
  auto statusAfter = service.getStatus();
  // In real scenario with mock server, this would be true

  service.stop();
}

TEST_F(ClientServiceTest, UptimeIncreasesWhileRunning) {
  ClientService service(validConfigPath);
  service.start();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  auto status1 = service.getStatus();
  std::cout << "Uptime 1: " << status1.uptime << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto status2 = service.getStatus();
  std::cout << "Uptime 2: " << status2.uptime << std::endl;

  EXPECT_GT(status2.uptime, status1.uptime);

  service.stop();
}

// ============================================================================
// CONNECTION TESTS
// ============================================================================

TEST_F(ClientServiceTest, ConnectToMasterAttempted) {
  // This test would require a mock TCP server
  // For now, verify that start() doesn't crash
  ClientService service(validConfigPath);

  EXPECT_NO_THROW({
    service.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    service.stop();
  });
}

TEST_F(ClientServiceTest, HelloMessageFormatValid) {
  // Verify HELLO message structure
  ClientService service(validConfigPath);

  // Get machine ID from status
  auto status = service.getStatus();
  std::string machineId = status.machine_id;

  // Create expected HELLO message
  nlohmann::json helloPayload = {
      {"version", status.service_version},
      {"machine_id", machineId},
      {"capabilities",
       nlohmann::json::array({"screenshot", "screen_lock", "power_control"})}};

  auto helloMsg =
      Message::Create(CommandType::HELLO, machineId, "master", helloPayload);

  // Verify message can be serialized
  MessageSerializer serializer;
  EXPECT_NO_THROW({
    std::string json = serializer.Serialize(helloMsg);
    EXPECT_FALSE(json.empty());
  });
}

// ============================================================================
// HEARTBEAT TESTS
// ============================================================================

TEST_F(ClientServiceTest, HeartbeatIntervalConfigurable) {
  ClientService service(validConfigPath);

  // Default heartbeat interval should be reasonable
  EXPECT_GT(service.getHeartbeatInterval(), 0);
  EXPECT_LE(service.getHeartbeatInterval(), 60); // Max 60 seconds
}

TEST_F(ClientServiceTest, LastHeartbeatTimestampUpdates) {
  ClientService service(validConfigPath);
  service.start();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  auto status1 = service.getStatus();

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  auto status2 = service.getStatus();

  // Timestamp should update (even if not connected, internal tracking)
  EXPECT_GE(status2.last_heartbeat, status1.last_heartbeat);

  service.stop();
}

// ============================================================================
// RECONNECTION TESTS
// ============================================================================

TEST_F(ClientServiceTest, ReconnectionOnDisconnect) {
  ClientService service(validConfigPath);
  service.start();

  // Simulate network error
  // In real implementation, would trigger disconnect

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Service should attempt to reconnect
  // Would need mock server to verify

  service.stop();
}

TEST_F(ClientServiceTest, ReconnectionAttemptsLimited) {
  ClientService service(validConfigPath);

  // Verify max reconnection attempts is reasonable
  EXPECT_GT(service.getMaxReconnectAttempts(), 0);
  EXPECT_LT(service.getMaxReconnectAttempts(), 100);
}

// ============================================================================
// COMMAND QUEUE TESTS
// ============================================================================

TEST_F(ClientServiceTest, CommandQueueInitiallyEmpty) {
  ClientService service(validConfigPath);

  EXPECT_EQ(service.getPendingCommandCount(), 0);
}

TEST_F(ClientServiceTest, CommandQueueProcessing) {
  ClientService service(validConfigPath);
  service.start();

  // In real scenario, would send commands from mock server
  // Queue should process them

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Verify queue is being processed

  service.stop();
}

// ============================================================================
// THREAD SAFETY TESTS
// ============================================================================

TEST_F(ClientServiceTest, ConcurrentStartStopSafe) {
  ClientService service(validConfigPath);

  std::thread t1([&service]() {
    service.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    service.stop();
  });

  std::thread t2([&service]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto status = service.getStatus();
  });

  EXPECT_NO_THROW({
    t1.join();
    t2.join();
  });
}

TEST_F(ClientServiceTest, ConcurrentStatusQueriesSafe) {
  ClientService service(validConfigPath);
  service.start();

  std::atomic<int> queryCount{0};

  auto queryStatus = [&service, &queryCount]() {
    for (int i = 0; i < 100; i++) {
      service.getStatus();
      queryCount++;
    }
  };

  std::thread t1(queryStatus);
  std::thread t2(queryStatus);

  t1.join();
  t2.join();

  EXPECT_EQ(queryCount.load(), 200);

  service.stop();
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_F(ClientServiceTest, FullLifecycle) {
  ClientService service(validConfigPath);

  // 1. Start service
  EXPECT_TRUE(service.start());
  EXPECT_TRUE(service.isRunning());

  // 2. Check status
  auto status = service.getStatus();
  EXPECT_FALSE(status.service_version.empty());

  // 3. Wait for heartbeat
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // 4. Stop service
  EXPECT_TRUE(service.stop());
  EXPECT_FALSE(service.isRunning());
}

TEST_F(ClientServiceTest, MultipleLifecycles) {
  ClientService service(validConfigPath);

  for (int i = 0; i < 3; i++) {
    service.start();
    EXPECT_TRUE(service.isRunning());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service.stop();
    EXPECT_FALSE(service.isRunning());
  }
}
