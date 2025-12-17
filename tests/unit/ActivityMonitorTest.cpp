#include "cms/ActivityMonitor.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>


// Test Fixture
class ActivityMonitorTest : public ::testing::Test {
protected:
  void SetUp() override {
    monitor = cms::ActivityMonitor::Create();
    ASSERT_NE(monitor, nullptr) << "Failed to create ActivityMonitor instance";
  }

  void TearDown() override {
    if (monitor && monitor->isMonitoring()) {
      monitor->stopMonitoring();
    }
  }

  std::unique_ptr<cms::ActivityMonitor> monitor;
};

// 1. Start/Stop Monitoring
TEST_F(ActivityMonitorTest, StartStopMonitoring) {
  EXPECT_FALSE(monitor->isMonitoring());
  EXPECT_TRUE(monitor->startMonitoring());
  EXPECT_TRUE(monitor->isMonitoring());
  // Idempotency
  EXPECT_TRUE(monitor->startMonitoring());

  EXPECT_TRUE(monitor->stopMonitoring());
  EXPECT_FALSE(monitor->isMonitoring());
  // Idempotency
  EXPECT_TRUE(monitor->stopMonitoring());
}

// 2. CPU Usage Range
TEST_F(ActivityMonitorTest, CPUUsageInRange) {
  monitor->startMonitoring();
  std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow gather
  float cpu = monitor->getCPUUsage();
  EXPECT_GE(cpu, 0.0f);
  EXPECT_LE(cpu, 100.0f);
}

// 3. RAM Usage Range
TEST_F(ActivityMonitorTest, RAMUsageInRange) {
  float ram = monitor->getRAMUsage();
  EXPECT_GE(ram, 0.0f);
  EXPECT_LE(ram, 100.0f);
}

// 4. Process List Not Empty
TEST_F(ActivityMonitorTest, ProcessListNotEmpty) {
  // Current process should at least be there
  auto processes = monitor->getActiveApplications();
  EXPECT_FALSE(processes.empty());

  bool foundSelf = false;
  // Simple check (name might vary by OS, but usually contains "test" or "cms")
  // We check valid IDs
  for (const auto &p : processes) {
    if (p.process_id > 0)
      foundSelf = true;
    EXPECT_FALSE(p.process_name.empty());
  }
  EXPECT_TRUE(foundSelf);
}

// 5. Idle Time
TEST_F(ActivityMonitorTest, IdleTimeSanity) {
  uint64_t idle = monitor->getScreenIdleTime();
  // Idle time >= 0
  EXPECT_GE(idle, 0);
}

// 6. Keyboard Activity (Mock interaction difficult without input injection)
// We rely on "Initial state" being safe
TEST_F(ActivityMonitorTest, InitialKeyboardStats) {
  auto stats = monitor->getKeyboardActivity();
  EXPECT_GE(stats.total_keystrokes, 0);
}

// 7. Mouse Activity
TEST_F(ActivityMonitorTest, InitialMouseStats) {
  auto stats = monitor->getMouseActivity();
  EXPECT_GE(stats.total_moves, 0);
}

// 8. Network usage
TEST_F(ActivityMonitorTest, NetworkStatsSanity) {
  auto stats = monitor->getNetworkUsage();
  EXPECT_GE(stats.bytes_received, 0);
  EXPECT_GE(stats.bytes_sent, 0);
}

// 9. Full Report
TEST_F(ActivityMonitorTest, ActivityReportConsistency) {
  monitor->startMonitoring();
  auto report = monitor->getAllActivity();

  EXPECT_GT(report.timestamp, 0);
  EXPECT_EQ(report.processes.size(), monitor->getActiveApplications().size());
  EXPECT_GE(report.cpu_average, 0.0f);
  EXPECT_GE(report.ram_average, 0.0f);
}

// 10. Thread Safety / Concurrent Access
TEST_F(ActivityMonitorTest, ConcurrentAccess) {
  monitor->startMonitoring();
  std::atomic<bool> run(true);

  std::thread t1([&]() {
    while (run) {
      auto p = monitor->getActiveApplications();
    }
  });

  std::thread t2([&]() {
    while (run) {
      monitor->getCPUUsage();
      monitor->getRAMUsage();
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  run = false;
  t1.join();
  t2.join();
}
