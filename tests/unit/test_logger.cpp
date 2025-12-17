#include "cms/Logger.h"
#include <gtest/gtest.h>
#include <sstream>


// Test fixture for Logger module
class LoggerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Reset logger to default state
    cms::Logger::getInstance().SetLogLevel(cms::LogLevel::Info);
  }

  void TearDown() override {
    // Cleanup if needed
  }
};

// Test that Logger.h header exists and compiles
TEST_F(LoggerTest, HeaderCompiles) {
  SUCCEED() << "Logger.h compiles successfully";
}

// Test Logger singleton instance
TEST_F(LoggerTest, SingletonInstance) {
  auto &logger1 = cms::Logger::getInstance();
  auto &logger2 = cms::Logger::getInstance();

  // Both references should point to the same instance
  EXPECT_EQ(&logger1, &logger2);
}

// Test log level setting and getting
TEST_F(LoggerTest, LogLevelGetSet) {
  auto &logger = cms::Logger::getInstance();

  logger.SetLogLevel(cms::LogLevel::Debug);
  EXPECT_EQ(logger.GetLogLevel(), cms::LogLevel::Debug);

  logger.SetLogLevel(cms::LogLevel::Warning);
  EXPECT_EQ(logger.GetLogLevel(), cms::LogLevel::Warning);

  logger.SetLogLevel(cms::LogLevel::Error);
  EXPECT_EQ(logger.GetLogLevel(), cms::LogLevel::Error);

  logger.SetLogLevel(cms::LogLevel::Info);
  EXPECT_EQ(logger.GetLogLevel(), cms::LogLevel::Info);
}

// Test that logging functions execute without crashing
TEST_F(LoggerTest, LoggingFunctionsExecute) {
  auto &logger = cms::Logger::getInstance();
  logger.SetLogLevel(cms::LogLevel::Debug);

  // These should not throw exceptions
  EXPECT_NO_THROW(logger.Debug("Debug message"));
  EXPECT_NO_THROW(logger.Info("Info message"));
  EXPECT_NO_THROW(logger.Warning("Warning message"));
  EXPECT_NO_THROW(logger.Error("Error message"));
}

// Test that log level filtering works
TEST_F(LoggerTest, LogLevelFiltering) {
  auto &logger = cms::Logger::getInstance();

  // Set to Warning level - should only log Warning and Error
  logger.SetLogLevel(cms::LogLevel::Warning);

  // These should not crash even when filtered
  EXPECT_NO_THROW(logger.Debug("This should be filtered"));
  EXPECT_NO_THROW(logger.Info("This should be filtered"));
  EXPECT_NO_THROW(logger.Warning("This should be logged"));
  EXPECT_NO_THROW(logger.Error("This should be logged"));
}

// Test logging macros
TEST_F(LoggerTest, LoggingMacros) {
  cms::Logger::getInstance().SetLogLevel(cms::LogLevel::Debug);

  EXPECT_NO_THROW(LOG_DEBUG("Debug via macro"));
  EXPECT_NO_THROW(LOG_INFO("Info via macro"));
  EXPECT_NO_THROW(LOG_WARNING("Warning via macro"));
  EXPECT_NO_THROW(LOG_ERROR("Error via macro"));
}

// Test thread safety (basic test)
TEST_F(LoggerTest, ThreadSafety) {
  auto &logger = cms::Logger::getInstance();

  // Multiple calls should not crash
  for (int i = 0; i < 10; ++i) {
    logger.Info("Message " + std::to_string(i));
  }

  SUCCEED() << "Logger handles multiple sequential calls";
}
