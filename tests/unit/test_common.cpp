#include <gtest/gtest.h>
#include "cms/Common.h"

// Test fixture for Common module
class CommonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test that Common.h header exists and compiles
TEST_F(CommonTest, HeaderCompiles) {
    SUCCEED() << "Common.h compiles successfully";
}

// Test version constants
TEST_F(CommonTest, VersionConstants) {
    EXPECT_STREQ(cms::VERSION, "1.0.0");
    EXPECT_EQ(cms::VERSION_MAJOR, 1);
    EXPECT_EQ(cms::VERSION_MINOR, 0);
    EXPECT_EQ(cms::VERSION_PATCH, 0);
}

// Test platform detection
TEST_F(CommonTest, PlatformDetection) {
    EXPECT_NE(cms::PLATFORM_NAME, nullptr);
    EXPECT_GT(std::strlen(cms::PLATFORM_NAME), 0);
    
    // Verify platform is one of the expected values
    bool validPlatform = 
        (std::strcmp(cms::PLATFORM_NAME, "Windows") == 0) ||
        (std::strcmp(cms::PLATFORM_NAME, "macOS") == 0) ||
        (std::strcmp(cms::PLATFORM_NAME, "Linux") == 0) ||
        (std::strcmp(cms::PLATFORM_NAME, "Unknown") == 0);
    
    EXPECT_TRUE(validPlatform) << "Platform name is: " << cms::PLATFORM_NAME;
}

// Test StatusCode enum
TEST_F(CommonTest, StatusCodeEnum) {
    EXPECT_EQ(static_cast<int>(cms::StatusCode::Success), 0);
    EXPECT_NE(static_cast<int>(cms::StatusCode::Error), 0);
}

// Test StatusCodeToString function
TEST_F(CommonTest, StatusCodeToString) {
    EXPECT_STREQ(cms::StatusCodeToString(cms::StatusCode::Success), "Success");
    EXPECT_STREQ(cms::StatusCodeToString(cms::StatusCode::Error), "Error");
    EXPECT_STREQ(cms::StatusCodeToString(cms::StatusCode::InvalidArgument), "Invalid Argument");
    EXPECT_STREQ(cms::StatusCodeToString(cms::StatusCode::NotFound), "Not Found");
    EXPECT_STREQ(cms::StatusCodeToString(cms::StatusCode::AlreadyExists), "Already Exists");
    EXPECT_STREQ(cms::StatusCodeToString(cms::StatusCode::PermissionDenied), "Permission Denied");
    EXPECT_STREQ(cms::StatusCodeToString(cms::StatusCode::NetworkError), "Network Error");
}

// Test type aliases
TEST_F(CommonTest, TypeAliases) {
    cms::String str = "test";
    EXPECT_EQ(str, "test");
    
    cms::byte b = 255;
    EXPECT_EQ(b, 255);
}
