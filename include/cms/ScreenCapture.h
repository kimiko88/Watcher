#ifndef CMS_SCREEN_CAPTURE_H
#define CMS_SCREEN_CAPTURE_H

#include "Common.h"
#include "Platform.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace cms {
namespace capture {

// ============================================================================
// ENUMS
// ============================================================================

// Pixel format
enum class PixelFormat {
    RGBA32,     // 32-bit RGBA (8 bits per channel)
    BGRA32,     // 32-bit BGRA (Windows native)
    RGB24,      // 24-bit RGB (no alpha)
    BGR24       // 24-bit BGR
};

// Compression types
enum class CompressionType {
    NONE,       // No compression
    RLE,        // Run-Length Encoding
    ZSTD,       // Zstandard (future)
    JPEG        // JPEG lossy (future)
};

// ============================================================================
// STRUCTURES
// ============================================================================

// Screen frame data
struct ScreenFrame {
    uint64_t timestamp;             // Capture timestamp (milliseconds since epoch)
    std::vector<uint8_t> pixel_data; // Raw pixel data
    int width;                      // Frame width in pixels
    int height;                     // Frame height in pixels
    int pitch;                      // Bytes per row (width * bytes_per_pixel)
    PixelFormat format;             // Pixel format
};

// Compressed frame data
struct CompressedFrame {
    std::vector<uint8_t> data;      // Compressed data
    size_t original_size;           // Original uncompressed size
    size_t compressed_size;         // Compressed data size
    float compression_ratio;        // Compression ratio (original / compressed)
    PixelFormat format;             // Original pixel format
    CompressionType compression;    // Compression type used
};

// ============================================================================
// SCREEN CAPTURE SERVICE
// ============================================================================

class ScreenCaptureService {
public:
    // Constructor - requires platform manager for screen capture
    explicit ScreenCaptureService(platform::Platform* platform);
    
    // Destructor
    ~ScreenCaptureService();
    
    // Capture screen and store in internal buffer
    // Returns reference to the captured frame
    ScreenFrame captureScreen();
    
    // Get pointer to current frame buffer
    // Returns nullptr if no frame has been captured
    const uint8_t* getFrameBuffer() const;
    
    // Get size of current frame buffer in bytes
    size_t getFrameSize() const;
    
    // Get frame dimensions
    int getWidth() const;
    int getHeight() const;
    
    // Get pixel format
    PixelFormat getFormat() const;
    
    // Compress current frame
    // Throws std::runtime_error if no frame has been captured
    CompressedFrame compress(CompressionType type);
    
    // Get compression ratio of last compression
    float getCompressionRatio() const;
    
    // Static compression/decompression utilities
    static CompressedFrame compressRLE(const ScreenFrame& frame);
    static std::vector<uint8_t> decompressRLE(const CompressedFrame& compressed, int width, int height);
    
private:
    platform::Platform* platform_;
    ScreenFrame current_frame_;
    CompressedFrame last_compressed_;
    float last_compression_ratio_;
    
    // Helper to get current timestamp
    static uint64_t getCurrentTimestamp();
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Get bytes per pixel for a given format
inline int getBytesPerPixel(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGBA32:
        case PixelFormat::BGRA32:
            return 4;
        case PixelFormat::RGB24:
        case PixelFormat::BGR24:
            return 3;
        default:
            return 4;
    }
}

// Convert pixel format to string
inline const char* pixelFormatToString(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGBA32: return "RGBA32";
        case PixelFormat::BGRA32: return "BGRA32";
        case PixelFormat::RGB24: return "RGB24";
        case PixelFormat::BGR24: return "BGR24";
        default: return "Unknown";
    }
}

// Convert compression type to string
inline const char* compressionTypeToString(CompressionType type) {
    switch (type) {
        case CompressionType::NONE: return "NONE";
        case CompressionType::RLE: return "RLE";
        case CompressionType::ZSTD: return "ZSTD";
        case CompressionType::JPEG: return "JPEG";
        default: return "Unknown";
    }
}

} // namespace capture
} // namespace cms

#endif // CMS_SCREEN_CAPTURE_H
