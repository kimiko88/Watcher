#include <gtest/gtest.h>
#include "cms/ScreenCapture.h"
#include "cms/Platform.h"
#include <thread>
#include <chrono>

using namespace cms::capture;
using namespace cms::platform;

// Helper to create test pattern
class TestPatternGenerator {
public:
    static std::vector<uint8_t> createSolidColor(int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        std::vector<uint8_t> data(width * height * 4);
        for (size_t i = 0; i < data.size(); i += 4) {
            data[i] = r;
            data[i + 1] = g;
            data[i + 2] = b;
            data[i + 3] = a;
        }
        return data;
    }
    
    static std::vector<uint8_t> createCheckerboard(int width, int height, int squareSize = 32) {
        std::vector<uint8_t> data(width * height * 4);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool isWhite = ((x / squareSize) + (y / squareSize)) % 2 == 0;
                uint8_t color = isWhite ? 255 : 0;
                
                size_t idx = (y * width + x) * 4;
                data[idx] = color;
                data[idx + 1] = color;
                data[idx + 2] = color;
                data[idx + 3] = 255;
            }
        }
        return data;
    }
};

// Test fixture
class ScreenCaptureTest : public ::testing::Test {
protected:
    void SetUp() override {
        platform = getPlatformInstance();
        service = std::make_unique<ScreenCaptureService>(platform.get());
    }
    
    void TearDown() override {
        service.reset();
        platform.reset();
    }
    
    std::unique_ptr<Platform> platform;
    std::unique_ptr<ScreenCaptureService> service;
};

// ============================================================================
// INITIALIZATION TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, ServiceInitialization) {
    EXPECT_NE(service, nullptr);
}

TEST_F(ScreenCaptureTest, ServiceRequiresPlatform) {
    EXPECT_THROW({
        ScreenCaptureService badService(nullptr);
    }, std::invalid_argument);
}

// ============================================================================
// CAPTURE TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, CaptureScreenReturnsValidFrame) {
    auto frame = service->captureScreen();
    
    EXPECT_FALSE(frame.pixel_data.empty());
    EXPECT_GT(frame.width, 0);
    EXPECT_GT(frame.height, 0);
    EXPECT_GT(frame.timestamp, 0);
}

TEST_F(ScreenCaptureTest, FrameDimensionsMatchScreenResolution) {
    auto frame = service->captureScreen();
    auto [width, height] = platform->getScreenDimensions();
    
    EXPECT_EQ(frame.width, width);
    EXPECT_EQ(frame.height, height);
}

TEST_F(ScreenCaptureTest, FrameFormatIsRGBA32) {
    auto frame = service->captureScreen();
    
    EXPECT_EQ(frame.format, PixelFormat::RGBA32);
}

TEST_F(ScreenCaptureTest, FrameSizeCalculation) {
    auto frame = service->captureScreen();
    
    size_t expected_size = frame.width * frame.height * 4;
    EXPECT_EQ(frame.pixel_data.size(), expected_size);
}

TEST_F(ScreenCaptureTest, PitchCalculation) {
    auto frame = service->captureScreen();
    
    // Pitch is bytes per row (width * 4 for RGBA)
    EXPECT_EQ(frame.pitch, frame.width * 4);
}

// ============================================================================
// BUFFER ACCESS TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, GetFrameBufferReturnsValidPointer) {
    service->captureScreen();
    
    const uint8_t* buffer = service->getFrameBuffer();
    EXPECT_NE(buffer, nullptr);
}

TEST_F(ScreenCaptureTest, GetFrameSizeMatchesExpectation) {
    auto frame = service->captureScreen();
    
    size_t size = service->getFrameSize();
    EXPECT_EQ(size, frame.pixel_data.size());
}

TEST_F(ScreenCaptureTest, GetWidthHeight) {
    service->captureScreen();
    
    auto [expected_width, expected_height] = platform->getScreenDimensions();
    
    EXPECT_EQ(service->getWidth(), expected_width);
    EXPECT_EQ(service->getHeight(), expected_height);
}

TEST_F(ScreenCaptureTest, GetFormatReturnsRGBA32) {
    service->captureScreen();
    
    EXPECT_EQ(service->getFormat(), PixelFormat::RGBA32);
}

// ============================================================================
// COMPRESSION - NONE TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, CompressionNoneReturnsUnmodifiedData) {
    auto frame = service->captureScreen();
    
    auto compressed = service->compress(CompressionType::NONE);
    
    EXPECT_EQ(compressed.data, frame.pixel_data);
    EXPECT_EQ(compressed.original_size, frame.pixel_data.size());
    EXPECT_EQ(compressed.compressed_size, frame.pixel_data.size());
    EXPECT_FLOAT_EQ(compressed.compression_ratio, 1.0f);
}

// ============================================================================
// COMPRESSION - RLE TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, CompressionRLEOnSolidColor) {
    // Create service with test pattern
    auto testData = TestPatternGenerator::createSolidColor(100, 100, 255, 0, 0);
    
    // Create a mock frame
    ScreenFrame testFrame;
    testFrame.pixel_data = testData;
    testFrame.width = 100;
    testFrame.height = 100;
    testFrame.pitch = 100 * 4;
    testFrame.format = PixelFormat::RGBA32;
    testFrame.timestamp = 123456;
    
    // Test RLE compression on solid color (should compress very well)
    auto compressed = ScreenCaptureService::compressRLE(testFrame);
    
    EXPECT_LT(compressed.compressed_size, compressed.original_size);
    EXPECT_GT(compressed.compression_ratio, 10.0f); // Solid color compresses >10x
}

TEST_F(ScreenCaptureTest, CompressionRLEOnCheckerboard) {
    auto testData = TestPatternGenerator::createCheckerboard(100, 100, 10);
    
    ScreenFrame testFrame;
    testFrame.pixel_data = testData;
    testFrame.width = 100;
    testFrame.height = 100;
    testFrame.pitch = 100 * 4;
    testFrame.format = PixelFormat::RGBA32;
    testFrame.timestamp = 123456;
    
    auto compressed = ScreenCaptureService::compressRLE(testFrame);
    
    // Checkerboard should still compress somewhat
    EXPECT_LT(compressed.compressed_size, compressed.original_size);
    EXPECT_GT(compressed.compression_ratio, 1.0f);
}

TEST_F(ScreenCaptureTest, CompressionRLEDecompression) {
    auto testData = TestPatternGenerator::createSolidColor(50, 50, 128, 128, 128);
    
    ScreenFrame testFrame;
    testFrame.pixel_data = testData;
    testFrame.width = 50;
    testFrame.height = 50;
    testFrame.pitch = 50 * 4;
    testFrame.format = PixelFormat::RGBA32;
    testFrame.timestamp = 123456;
    
    auto compressed = ScreenCaptureService::compressRLE(testFrame);
    auto decompressed = ScreenCaptureService::decompressRLE(compressed, testFrame.width, testFrame.height);
    
    ASSERT_EQ(decompressed.size(), testData.size());
    EXPECT_EQ(decompressed, testData);
}

// ============================================================================
// COMPRESSION RATIO TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, GetCompressionRatioAfterCompress) {
    service->captureScreen();
    service->compress(CompressionType::RLE);
    
    float ratio = service->getCompressionRatio();
    EXPECT_GT(ratio, 0.0f);
}

TEST_F(ScreenCaptureTest, CompressionRatioCalculation) {
    auto testData = TestPatternGenerator::createSolidColor(100, 100, 255, 255, 255);
    
    ScreenFrame testFrame;
    testFrame.pixel_data = testData;
    testFrame.width = 100;
    testFrame.height = 100;
    testFrame.pitch = 100 * 4;
    testFrame.format = PixelFormat::RGBA32;
    testFrame.timestamp = 123456;
    
    auto compressed = ScreenCaptureService::compressRLE(testFrame);
    
    float expected_ratio = static_cast<float>(compressed.original_size) / compressed.compressed_size;
    EXPECT_FLOAT_EQ(compressed.compression_ratio, expected_ratio);
}

// ============================================================================
// THREAD SAFETY TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, ConcurrentCaptures) {
    std::atomic<int> successCount{0};
    
    auto captureTask = [this, &successCount]() {
        for (int i = 0; i < 10; i++) {
            try {
                auto frame = service->captureScreen();
                if (!frame.pixel_data.empty()) {
                    successCount++;
                }
            } catch (...) {
                // Capture failed
            }
        }
    };
    
    std::thread t1(captureTask);
    std::thread t2(captureTask);
    
    t1.join();
    t2.join();
    
    // At least some captures should succeed
    EXPECT_GT(successCount.load(), 0);
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, CapturePerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    auto frame = service->captureScreen();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Capture should be fast (< 200ms for reasonable screen size)
    EXPECT_LT(duration.count(), 200);
}

TEST_F(ScreenCaptureTest, CompressPerformance) {
    service->captureScreen();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto compressed = service->compress(CompressionType::RLE);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Compression should be fast (< 150ms for RLE)
    EXPECT_LT(duration.count(), 150);
}

TEST_F(ScreenCaptureTest, CaptureAndCompressTotalPerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    service->captureScreen();
    service->compress(CompressionType::RLE);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Total should be < 300ms
    EXPECT_LT(duration.count(), 300);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_F(ScreenCaptureTest, CompressBeforeCaptureThrows) {
    EXPECT_THROW({
        service->compress(CompressionType::RLE);
    }, std::runtime_error);
}

TEST_F(ScreenCaptureTest, GetBufferBeforeCaptureReturnsNull) {
    const uint8_t* buffer = service->getFrameBuffer();
    EXPECT_EQ(buffer, nullptr);
}

TEST_F(ScreenCaptureTest, MultipleCapturesUpdateFrame) {
    auto frame1 = service->captureScreen();
    auto timestamp1 = frame1.timestamp;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    auto frame2 = service->captureScreen();
    auto timestamp2 = frame2.timestamp;
    
    EXPECT_GT(timestamp2, timestamp1);
}
