#include "cms/ScreenRecorder.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

namespace cms {
namespace fs = std::filesystem;

// Internal implementation class (Pimpl pattern)
class ScreenRecorder::Impl {
public:
  std::string output_directory;
  std::string current_filename;
  Quality quality = Quality::MEDIUM;
  uint32_t frame_rate = 30;
  bool recording = false;
  bool paused = false;

  // Stats tracking
  uint64_t frame_count = 0;
  uint64_t duration_seconds = 0;
  float file_size_mb = 0.0f;
  uint32_t frame_drops = 0;

  mutable std::mutex mutex;
  std::thread encoding_thread;

  explicit Impl(const std::string &dir) : output_directory(dir) {}

  ~Impl() {
    if (recording) {
      stopRecording();
    }
    if (encoding_thread.joinable()) {
      encoding_thread.join();
    }
  }

  bool startRecording(const std::string &filename, Quality qual) {
    std::lock_guard<std::mutex> lock(mutex);

    if (recording) {
      return false; // Already recording
    }

    current_filename = filename;
    quality = qual;
    recording = true;
    paused = false;
    frame_count = 0;
    duration_seconds = 0;
    frame_drops = 0;

    // Create output directory if it doesn't exist
    fs::create_directories(output_directory);

    // Start encoding thread (stub for now)
    encoding_thread = std::thread([this]() { encodingLoop(); });

    return true;
  }

  bool stopRecording() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!recording) {
      return false;
    }

    recording = false;
    paused = false;

    // Wait for encoding thread to finish
    if (encoding_thread.joinable()) {
      mutex.unlock();
      encoding_thread.join();
      mutex.lock();
    }

    // Create placeholder video file (for testing)
    createVideoFile();

    return true;
  }

  bool pauseRecording() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!recording || paused) {
      return false;
    }

    paused = true;
    return true;
  }

  bool resumeRecording() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!recording || !paused) {
      return false;
    }

    paused = false;
    return true;
  }

  bool cancelRecording() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!recording) {
      return false;
    }

    recording = false;
    paused = false;

    // Delete partial file
    std::string path = getOutputPath();
    if (fs::exists(path)) {
      fs::remove(path);
    }

    return true;
  }

  RecordingStats getStats() const {
    std::lock_guard<std::mutex> lock(mutex);

    RecordingStats stats;
    stats.filename = current_filename;
    stats.duration_seconds = duration_seconds;
    stats.frame_count = frame_count;
    stats.file_size_mb = file_size_mb;
    stats.fps_actual = frame_rate;
    stats.bitrate_kbps = calculateBitrate();
    stats.codec = getCodecName();
    stats.resolution = getResolution();
    stats.disk_free_mb = getDiskFreeSpace();
    stats.frame_drops = frame_drops;

    return stats;
  }

  std::string getOutputPath() const {
    std::string extension = getFileExtension();
    fs::path path = fs::path(output_directory) / (current_filename + extension);
    return path.string();
  }

private:
  void encodingLoop() {
    // Stub encoding loop - just simulate recording
    auto start_time = std::chrono::steady_clock::now();

    while (true) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        if (!recording)
          break;

        if (!paused) {
          frame_count++;

          auto now = std::chrono::steady_clock::now();
          duration_seconds =
              std::chrono::duration_cast<std::chrono::seconds>(now - start_time)
                  .count();
        }
      }

      // Simulate frame capture delay
      std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frame_rate));
    }
  }

  void createVideoFile() {
    // Create a placeholder video file for testing
    std::string path = getOutputPath();
    std::ofstream file(path, std::ios::binary);

    if (file.is_open()) {
      // Write minimal MP4/WebM header (just for file existence check)
      file << "PLACEHOLDER_VIDEO_DATA";
      file.close();

      // Update file size
      file_size_mb =
          static_cast<float>(fs::file_size(path)) / (1024.0f * 1024.0f);
    }
  }

  std::string getFileExtension() const {
    switch (quality) {
    case Quality::LOW:
    case Quality::MEDIUM:
      return ".webm"; // VP9
    case Quality::HIGH:
    case Quality::ULTRA:
      return ".mp4"; // H.265
    default:
      return ".mp4";
    }
  }

  std::string getCodecName() const {
    switch (quality) {
    case Quality::LOW:
    case Quality::MEDIUM:
      return "vp9";
    case Quality::HIGH:
    case Quality::ULTRA:
      return "hevc"; // H.265
    default:
      return "unknown";
    }
  }

  std::string getResolution() const {
    switch (quality) {
    case Quality::LOW:
      return "1280x720";
    case Quality::MEDIUM:
      return "1920x1080";
    case Quality::HIGH:
      return "2560x1440";
    case Quality::ULTRA:
      return "3840x2160";
    default:
      return "0x0";
    }
  }

  float calculateBitrate() const {
    if (duration_seconds == 0)
      return 0.0f;
    return (file_size_mb * 8 * 1024) / duration_seconds; // kbps
  }

  float getDiskFreeSpace() const {
    try {
      auto space = fs::space(output_directory);
      return static_cast<float>(space.available) / (1024.0f * 1024.0f);
    } catch (...) {
      return 0.0f;
    }
  }
};

// Public interface implementation
ScreenRecorder::ScreenRecorder(const std::string &output_directory)
    : pImpl_(new Impl(output_directory)) {}

ScreenRecorder::~ScreenRecorder() { delete pImpl_; }

bool ScreenRecorder::startRecording(const std::string &filename,
                                    Quality quality) {
  return pImpl_->startRecording(filename, quality);
}

bool ScreenRecorder::stopRecording() { return pImpl_->stopRecording(); }

bool ScreenRecorder::isRecording() const {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);
  return pImpl_->recording;
}

bool ScreenRecorder::pauseRecording() { return pImpl_->pauseRecording(); }

bool ScreenRecorder::resumeRecording() { return pImpl_->resumeRecording(); }

bool ScreenRecorder::setFrameRate(uint32_t fps) {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);

  if (pImpl_->recording) {
    return false; // Cannot change during recording
  }

  if (fps < 1 || fps > 120) {
    return false; // Invalid range
  }

  pImpl_->frame_rate = fps;
  return true;
}

bool ScreenRecorder::setQuality(Quality quality) {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);

  if (pImpl_->recording) {
    return false; // Cannot change during recording
  }

  pImpl_->quality = quality;
  return true;
}

RecordingStats ScreenRecorder::getRecordingStats() const {
  return pImpl_->getStats();
}

std::string ScreenRecorder::getOutputPath() const {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);
  return pImpl_->getOutputPath();
}

bool ScreenRecorder::cancelRecording() { return pImpl_->cancelRecording(); }

} // namespace cms
