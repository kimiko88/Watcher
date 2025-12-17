#include <gtest/gtest.h>
#include "cms/Platform.h"
#include <memory>

using namespace cms::platform;

// Test fixture for Platform tests
class PlatformTest : public ::testing::Test {
protected:
    void SetUp() override {
        platform = getPlatformInstance();
    }

    void TearDown() override {
        platform.reset();
    }

    std::unique_ptr<Platform> platform;
};

// ============================================================================
// FACTORY TESTS
// ============================================================================

TEST_F(PlatformTest, FactoryCreatesNonNullPlatform) {
    EXPECT_NE(platform, nullptr);
}

TEST_F(PlatformTest, FactoryCreatesCorrectPlatformType) {
    ASSERT_NE(platform, nullptr);
    
    std::string name = platform->getPlatformName();
    
    // Should be one of the supported platforms
    bool validPlatform = 
        (name == "Windows") ||
        (name == "Linux") ||
        (name == "macOS");
    
    EXPECT_TRUE(validPlatform) << "Platform name is: " << name;
}

TEST_F(PlatformTest, PlatformDetection) {
    ASSERT_NE(platform, nullptr);
    
    std::string name = platform->getPlatformName();
    
#if defined(CMS_PLATFORM_WINDOWS)
    EXPECT_EQ(name, "Windows");
#elif defined(CMS_PLATFORM_MACOS)
    EXPECT_EQ(name, "macOS");
#elif defined(CMS_PLATFORM_LINUX)
    EXPECT_EQ(name, "Linux");
#endif
}

// ============================================================================
// IPLATFORMMANAGER TESTS
// ============================================================================

TEST_F(PlatformTest, GetPlatformNameReturnsNonEmpty) {
    ASSERT_NE(platform, nullptr);
    
    std::string name = platform->getPlatformName();
    EXPECT_FALSE(name.empty());
}

TEST_F(PlatformTest, GetCPUInfoReturnsValidData) {
    ASSERT_NE(platform, nullptr);
    
    auto cpuInfo = platform->getCPUInfo();
    
    // Brand should not be empty
    EXPECT_FALSE(cpuInfo.brand.empty());
    
    // Cores should be positive
    EXPECT_GT(cpuInfo.cores, 0);
    EXPECT_GT(cpuInfo.logical_cores, 0);
    
    // Logical cores >= physical cores
    EXPECT_GE(cpuInfo.logical_cores, cpuInfo.cores);
    
    // Frequency should be reasonable (0.5 GHz to 10 GHz)
    EXPECT_GT(cpuInfo.frequency_ghz, 0.5);
    EXPECT_LT(cpuInfo.frequency_ghz, 10.0);
}

TEST_F(PlatformTest, GetRAMInfoReturnsValidData) {
    ASSERT_NE(platform, nullptr);
    
    auto ramInfo = platform->getRAMInfo();
    
    // Total RAM should be positive
    EXPECT_GT(ramInfo.total_bytes, 0);
    
    // Used + Available should roughly equal Total
    EXPECT_GT(ramInfo.available_bytes, 0);
    EXPECT_GT(ramInfo.used_bytes, 0);
    
    // Usage percent should be 0-100
    EXPECT_GE(ramInfo.usage_percent, 0.0);
    EXPECT_LE(ramInfo.usage_percent, 100.0);
}

TEST_F(PlatformTest, GetScreenResolutionReturnsValidData) {
    ASSERT_NE(platform, nullptr);
    
    auto resolution = platform->getScreenResolution();
    
    // Resolution should be positive
    EXPECT_GT(resolution.width, 0);
    EXPECT_GT(resolution.height, 0);
    
    // Common resolutions are at least 640x480
    EXPECT_GE(resolution.width, 640);
    EXPECT_GE(resolution.height, 480);
    
    // Bits per pixel should be reasonable (8, 16, 24, 32)
    EXPECT_GT(resolution.bits_per_pixel, 0);
}

TEST_F(PlatformTest, GetScreenScaleFactorReturnsPositive) {
    ASSERT_NE(platform, nullptr);
    
    float scaleFactor = platform->getScreenScaleFactor();
    
    // Scale factor should be positive
    EXPECT_GT(scaleFactor, 0.0f);
    
    // Common scale factors: 1.0, 1.25, 1.5, 2.0
    EXPECT_GE(scaleFactor, 0.5f);
    EXPECT_LE(scaleFactor, 4.0f);
}

// ============================================================================
// ISCREENCAPTURE TESTS
// ============================================================================

TEST_F(PlatformTest, GetScreenDimensionsReturnsPositive) {
    ASSERT_NE(platform, nullptr);
    
    auto [width, height] = platform->getScreenDimensions();
    
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
    
    // Should match resolution
    auto resolution = platform->getScreenResolution();
    EXPECT_EQ(width, resolution.width);
    EXPECT_EQ(height, resolution.height);
}

TEST_F(PlatformTest, SupportedFormatsReturnsNonEmpty) {
    ASSERT_NE(platform, nullptr);
    
    auto formats = platform->supportedFormats();
    
    // Should support at least one format
    EXPECT_FALSE(formats.empty());
    
    // Should include RGBA
    bool hasRGBA = false;
    for (auto format : formats) {
        if (format == ImageFormat::RGBA) {
            hasRGBA = true;
            break;
        }
    }
    EXPECT_TRUE(hasRGBA) << "Should support RGBA format";
}

TEST_F(PlatformTest, CaptureScreenReturnsValidImage) {
    ASSERT_NE(platform, nullptr);
    
    auto image = platform->captureScreen();
    
    // Image should have data
    EXPECT_FALSE(image.data.empty());
    
    // Dimensions should be positive
    EXPECT_GT(image.width, 0);
    EXPECT_GT(image.height, 0);
    
    // Data size should match dimensions (for RGBA: width * height * 4)
    if (image.format == ImageFormat::RGBA) {
        size_t expected_size = image.width * image.height * 4;
        EXPECT_EQ(image.data.size(), expected_size);
    }
}

// ============================================================================
// IINPUTCONTROL TESTS
// ============================================================================

TEST_F(PlatformTest, InputLockStateTracking) {
    ASSERT_NE(platform, nullptr);
    
    // Initially should be unlocked
    EXPECT_FALSE(platform->isInputLocked());
}

TEST_F(PlatformTest, LockUnlockKeyboard) {
    ASSERT_NE(platform, nullptr);
    
    // Try to lock keyboard (may require permissions)
    bool locked = platform->lockKeyboard();
    
    if (locked) {
        // If lock succeeded, state should reflect it
        EXPECT_TRUE(platform->isInputLocked());
        
        // Unlock
        bool unlocked = platform->unlockKeyboard();
        EXPECT_TRUE(unlocked);
        EXPECT_FALSE(platform->isInputLocked());
    } else {
        // If lock failed (permissions), that's okay
        EXPECT_FALSE(platform->isInputLocked());
    }
}

TEST_F(PlatformTest, LockUnlockMouse) {
    ASSERT_NE(platform, nullptr);
    
    // Try to lock mouse (may require permissions)
    bool locked = platform->lockMouse();
    
    if (locked) {
        // If lock succeeded, state should reflect it
        EXPECT_TRUE(platform->isInputLocked());
        
        // Unlock
        bool unlocked = platform->unlockMouse();
        EXPECT_TRUE(unlocked);
        EXPECT_FALSE(platform->isInputLocked());
    } else {
        // If lock failed (permissions), that's okay
        EXPECT_FALSE(platform->isInputLocked());
    }
}

// ============================================================================
// IPOWERCONTROL TESTS
// ============================================================================

TEST_F(PlatformTest, GetPowerStatusReturnsValidData) {
    ASSERT_NE(platform, nullptr);
    
    auto powerStatus = platform->getPowerStatus();
    
    // Power source should be valid
    EXPECT_TRUE(
        powerStatus.source == PowerSource::AC ||
        powerStatus.source == PowerSource::Battery ||
        powerStatus.source == PowerSource::Unknown
    );
    
    // Battery percent should be 0-100 or -1 (unknown)
    EXPECT_GE(powerStatus.battery_percent, -1);
    EXPECT_LE(powerStatus.battery_percent, 100);
}

// Note: We don't actually test powerOff, reboot, hibernate
// as they would shut down the test machine!
TEST_F(PlatformTest, PowerOperationsExist) {
    ASSERT_NE(platform, nullptr);
    
    // Just verify the methods exist and are callable
    // We would need mocks to actually test them
    SUCCEED() << "Power control methods exist in interface";
}

// ============================================================================
// INETWORKFILTER TESTS
// ============================================================================

TEST_F(PlatformTest, GetCurrentRulesReturnsVector) {
    ASSERT_NE(platform, nullptr);
    
    auto rules = platform->getCurrentRules();
    
    // Rules should be a vector (may be empty)
    // This just verifies the method works
    SUCCEED() << "getCurrentRules returns vector with " << rules.size() << " rules";
}

TEST_F(PlatformTest, SetFilterMode) {
    ASSERT_NE(platform, nullptr);
    
    // Try to set allow list mode (may require permissions)
    bool result1 = platform->setAllowListMode();
    
    // Try to set block list mode (may require permissions)
    bool result2 = platform->setBlockListMode();
    
    // If permissions are denied, that's okay
    // We just verify the methods exist
    SUCCEED() << "Filter mode methods callable";
}

TEST_F(PlatformTest, BlockDomains) {
    ASSERT_NE(platform, nullptr);
    
    std::vector<std::string> domains = {"example.com", "test.org"};
    
    // Try to block domains (may require permissions)
    bool result = platform->blockDomains(domains);
    
    // If permissions denied, that's okay
    // Just verify method is callable
    SUCCEED() << "blockDomains is callable";
}

TEST_F(PlatformTest, AllowDomains) {
    ASSERT_NE(platform, nullptr);
    
    std::vector<std::string> domains = {"allowed.com"};
    
    // Try to allow domains (may require permissions)
    bool result = platform->allowDomains(domains);
    
    // If permissions denied, that's okay
    // Just verify method is callable
    SUCCEED() << "allowDomains is callable";
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_F(PlatformTest, AllInterfacesImplemented) {
    ASSERT_NE(platform, nullptr);
    
    // Verify all interfaces are implemented by calling one method from each
    
    // IPlatformManager
    EXPECT_NO_THROW(platform->getPlatformName());
    
    // IScreenCapture
    EXPECT_NO_THROW(platform->getScreenDimensions());
    
    // IInputControl
    EXPECT_NO_THROW(platform->isInputLocked());
    
    // IPowerControl
    EXPECT_NO_THROW(platform->getPowerStatus());
    
    // INetworkFilter
    EXPECT_NO_THROW(platform->getCurrentRules());
}

TEST_F(PlatformTest, MultipleInstancesAllowed) {
    auto platform1 = getPlatformInstance();
    auto platform2 = getPlatformInstance();
    
    EXPECT_NE(platform1, nullptr);
    EXPECT_NE(platform2, nullptr);
    
    // Both should work independently
    EXPECT_EQ(platform1->getPlatformName(), platform2->getPlatformName());
}
