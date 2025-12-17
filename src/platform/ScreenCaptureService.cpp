#include "cms/ScreenCapture.h"
#include "cms/Logger.h"
#include <chrono>
#include <stdexcept>
#include <cstring>

namespace cms {
namespace capture {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ScreenCaptureService::ScreenCaptureService(platform::Platform* platform)
    : platform_(platform)
    , last_compression_ratio_(0.0f)
{
    if (!platform_) {
        throw std::invalid_argument("Platform cannot be null");
    }
    
    LOG_INFO("ScreenCaptureService initialized");
}

ScreenCaptureService::~ScreenCaptureService() {
    LOG_INFO("ScreenCaptureService destroyed");
}

// ============================================================================
// CAPTURE METHODS
// ============================================================================

ScreenFrame ScreenCaptureService::captureScreen() {
    LOG_DEBUG("Capturing screen...");
    
    // Use platform layer to capture screen
    auto platformImage = platform_->captureScreen();
    
    // Convert platform image to our format
    current_frame_.timestamp = getCurrentTimestamp();
    current_frame_.pixel_data = std::move(platformImage.data);
    current_frame_.width = platformImage.width;
    current_frame_.height = platformImage.height;
    current_frame_.format = PixelFormat::RGBA32; // Platform returns RGBA
    current_frame_.pitch = current_frame_.width * getBytesPerPixel(current_frame_.format);
    
    LOG_DEBUG("Screen captured: " + std::to_string(current_frame_.width) + "x" + 
              std::to_string(current_frame_.height));
    
    return current_frame_;
}

const uint8_t* ScreenCaptureService::getFrameBuffer() const {
    if (current_frame_.pixel_data.empty()) {
        return nullptr;
    }
    return current_frame_.pixel_data.data();
}

size_t ScreenCaptureService::getFrameSize() const {
    return current_frame_.pixel_data.size();
}

int ScreenCaptureService::getWidth() const {
    return current_frame_.width;
}

int ScreenCaptureService::getHeight() const {
    return current_frame_.height;
}

PixelFormat ScreenCaptureService::getFormat() const {
    return current_frame_.format;
}

// ============================================================================
// COMPRESSION METHODS
// ============================================================================

CompressedFrame ScreenCaptureService::compress(CompressionType type) {
    if (current_frame_.pixel_data.empty()) {
        throw std::runtime_error("No frame captured. Call captureScreen() first.");
    }
    
    LOG_DEBUG(std::string("Compressing with ") + compressionTypeToString(type));
    
    CompressedFrame compressed;
    
    switch (type) {
        case CompressionType::NONE:
            compressed.data = current_frame_.pixel_data;
            compressed.original_size = current_frame_.pixel_data.size();
            compressed.compressed_size = current_frame_.pixel_data.size();
            compressed.compression_ratio = 1.0f;
            compressed.format = current_frame_.format;
            compressed.compression = type;
            break;
            
        case CompressionType::RLE:
            compressed = compressRLE(current_frame_);
            break;
            
        case CompressionType::ZSTD:
        case CompressionType::JPEG:
            LOG_WARNING(std::string(compressionTypeToString(type)) + " not implemented yet");
            throw std::runtime_error("Compression type not implemented");
            
        default:
            throw std::invalid_argument("Unknown compression type");
    }
    
    last_compressed_ = compressed;
    last_compression_ratio_ = compressed.compression_ratio;
    
    LOG_DEBUG("Compression complete. Ratio: " + std::to_string(compressed.compression_ratio) + "x");
    
    return compressed;
}

float ScreenCaptureService::getCompressionRatio() const {
    return last_compression_ratio_;
}

// ============================================================================
// RLE COMPRESSION
// ============================================================================

CompressedFrame ScreenCaptureService::compressRLE(const ScreenFrame& frame) {
    CompressedFrame compressed;
    compressed.original_size = frame.pixel_data.size();
    compressed.format = frame.format;
    compressed.compression = CompressionType::RLE;
    
    const uint8_t* src = frame.pixel_data.data();
    size_t srcSize = frame.pixel_data.size();
    
    // Reserve space for worst case (no compression)
    compressed.data.reserve(srcSize + srcSize / 128);
    
    // RLE encoding: [count][value] for runs of 4-byte pixels (RGBA)
    // count: 1 byte (max 255 repetitions)
    // value: 4 bytes (RGBA pixel)
    
    int bytesPerPixel = getBytesPerPixel(frame.format);
    size_t pixelCount = srcSize / bytesPerPixel;
    
    size_t i = 0;
    while (i < pixelCount) {
        // Find run length
        uint8_t run = 1;
        const uint8_t* currentPixel = src + i * bytesPerPixel;
        
        while (i + run < pixelCount && run < 255) {
            const uint8_t* nextPixel = src + (i + run) * bytesPerPixel;
            
            // Compare pixels
            bool same = true;
            for (int b = 0; b < bytesPerPixel; b++) {
                if (currentPixel[b] != nextPixel[b]) {
                    same = false;
                    break;
                }
            }
            
            if (!same) break;
            run++;
        }
        
        // Write run
        compressed.data.push_back(run);
        for (int b = 0; b < bytesPerPixel; b++) {
            compressed.data.push_back(currentPixel[b]);
        }
        
        i += run;
    }
    
    compressed.compressed_size = compressed.data.size();
    compressed.compression_ratio = static_cast<float>(compressed.original_size) / compressed.compressed_size;
    
    return compressed;
}

// ============================================================================
// RLE DECOMPRESSION
// ============================================================================

std::vector<uint8_t> ScreenCaptureService::decompressRLE(const CompressedFrame& compressed, int width, int height) {
    int bytesPerPixel = getBytesPerPixel(compressed.format);
    size_t expectedSize = width * height * bytesPerPixel;
    
    std::vector<uint8_t> decompressed;
    decompressed.reserve(expectedSize);
    
    const uint8_t* src = compressed.data.data();
    size_t srcSize = compressed.data.size();
    
    size_t i = 0;
    while (i < srcSize) {
        if (i + 1 + bytesPerPixel > srcSize) {
            throw std::runtime_error("Corrupt RLE data");
        }
        
        uint8_t run = src[i++];
        
        // Copy pixel data
        for (uint8_t r = 0; r < run; r++) {
            for (int b = 0; b < bytesPerPixel; b++) {
                decompressed.push_back(src[i + b]);
            }
        }
        
        i += bytesPerPixel;
    }
    
    if (decompressed.size() != expectedSize) {
        throw std::runtime_error("RLE decompression size mismatch");
    }
    
    return decompressed;
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

uint64_t ScreenCaptureService::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace capture
} // namespace cms
