#include "cms/Common.h"
#include "cms/Config.h"
#include "cms/Logger.h"
#include <gtest/gtest.h>

// Integration test to verify basic compilation and linking
class BuildIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup
  }

  void TearDown() override { cms::Config::Instance().Clear(); }
};

// Test that all headers compile together
TEST_F(BuildIntegrationTest, AllHeadersCompileTogether) {
  SUCCEED() << "All CMS headers compile and link successfully";
}

// Test that Common and Logger work together
TEST_F(BuildIntegrationTest, CommonAndLoggerIntegration) {
  auto &logger = cms::Logger::getInstance();
  logger.SetLogLevel(cms::LogLevel::Info);

  std::string msg = std::string("Platform: ") + cms::PLATFORM_NAME;
  EXPECT_NO_THROW(logger.Info(msg));

  msg = std::string("Version: ") + cms::VERSION;
  EXPECT_NO_THROW(logger.Info(msg));
}

// Test that Config and Common work together
TEST_F(BuildIntegrationTest, ConfigAndCommonIntegration) {
  auto &config = cms::Config::Instance();

  config.Set("version", cms::VERSION);
  config.Set("platform", cms::PLATFORM_NAME);

  auto version = config.Get("version");
  ASSERT_TRUE(version.has_value());
  EXPECT_EQ(version.value(), cms::VERSION);

  auto platform = config.Get("platform");
  ASSERT_TRUE(platform.has_value());
  EXPECT_EQ(platform.value(), cms::PLATFORM_NAME);
}

// Test Config functionality
TEST_F(BuildIntegrationTest, ConfigOperations) {
  auto &config = cms::Config::Instance();

  // Test Set and Get
  config.Set("test.key", "test.value");
  auto value = config.Get("test.key");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(value.value(), "test.value");

  // Test Has
  EXPECT_TRUE(config.Has("test.key"));
  EXPECT_FALSE(config.Has("nonexistent.key"));

  // Test GetOr with existing key
  EXPECT_EQ(config.GetOr("test.key", "default"), "test.value");

  // Test GetOr with non-existing key
  EXPECT_EQ(config.GetOr("nonexistent.key", "default"), "default");

  // Test Size
  size_t initialSize = config.Size();
  config.Set("another.key", "another.value");
  EXPECT_EQ(config.Size(), initialSize + 1);

  // Test Remove
  config.Remove("test.key");
  EXPECT_FALSE(config.Has("test.key"));

  // Test Clear
  config.Clear();
  EXPECT_EQ(config.Size(), 0);
}

// Test that all components can be used together in a realistic scenario
TEST_F(BuildIntegrationTest, RealisticScenario) {
  auto &logger = cms::Logger::getInstance();
  auto &config = cms::Config::Instance();

  // Configure logging
  logger.SetLogLevel(cms::LogLevel::Debug);

  // Setup configuration
  config.Set("app.name", "CMS Test");
  config.Set("app.version", cms::VERSION);
  config.Set("app.platform", cms::PLATFORM_NAME);

  // Log configuration
  LOG_INFO("Application configuration:");
  LOG_INFO(std::string("  Name: ") + config.GetOr("app.name", "Unknown"));
  LOG_INFO(std::string("  Version: ") + config.GetOr("app.version", "Unknown"));
  LOG_INFO(std::string("  Platform: ") +
           config.GetOr("app.platform", "Unknown"));

  // Verify everything worked
  EXPECT_TRUE(config.Has("app.name"));
  EXPECT_EQ(logger.GetLogLevel(), cms::LogLevel::Debug);

  SUCCEED() << "Realistic integration scenario completed successfully";
}

// Test platform-specific compilation
TEST_F(BuildIntegrationTest, PlatformSpecificCode) {
#if defined(CMS_PLATFORM_WINDOWS)
  EXPECT_STREQ(cms::PLATFORM_NAME, "Windows");
  LOG_INFO("Running on Windows");
#elif defined(CMS_PLATFORM_MACOS)
  EXPECT_STREQ(cms::PLATFORM_NAME, "macOS");
  LOG_INFO("Running on macOS");
#elif defined(CMS_PLATFORM_LINUX)
  EXPECT_STREQ(cms::PLATFORM_NAME, "Linux");
  LOG_INFO("Running on Linux");
#else
  EXPECT_STREQ(cms::PLATFORM_NAME, "Unknown");
  LOG_WARNING("Running on unknown platform");
#endif

  SUCCEED() << "Platform-specific code compiles correctly";
}
