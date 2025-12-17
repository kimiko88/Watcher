#pragma once

#include <cstdint>
#include <string>

namespace cms {

// Quality presets for screen recording
enum class Quality {
  LOW,    // 720p @ 15fps, VP9 codec
  MEDIUM, // 1080p @ 24fps, VP9 codec
  HIGH,   // 1440p @ 30fps, H.265 codec
  ULTRA   // 2160p (4K) @ 60fps, H.265 codec
};

// Statistics about an active or completed recording
struct RecordingStats {
  std::string filename;
  uint64_t duration_seconds = 0;
  uint64_t frame_count = 0;
  float file_size_mb = 0.0f;
  float fps_actual = 0.0f;
  float bitrate_kbps = 0.0f;
  std::string codec;
  std::string resolution;
  float disk_free_mb = 0.0f;
  uint32_t frame_drops = 0;
};

// Screen recorder for capturing and encoding video
class ScreenRecorder {
public:
  // Constructor
  // @param output_directory: Directory where recordings will be saved
  explicit ScreenRecorder(const std::string &output_directory);

  // Destructor - ensures recording is stopped and cleaned up
  ~ScreenRecorder();

  // Start a new recording
  // @param filename: Name of the output file (without extension)
  // @param quality: Quality preset to use
  // @return true if recording started successfully, false otherwise
  bool startRecording(const std::string &filename, Quality quality);

  // Stop the current recording
  // @return true if stopped successfully, false if not recording
  bool stopRecording();

  // Check if currently recording
  // @return true if actively recording, false otherwise
  bool isRecording() const;

  // Pause the current recording (keeps file, stops capturing frames)
  // @return true if paused successfully, false if not recording or already
  // paused
  bool pauseRecording();

  // Resume a paused recording
  // @return true if resumed successfully, false if not paused
  bool resumeRecording();

  // Set the frame rate for recording
  // @param fps: Frames per second (1-120 range)
  // @return true if set successfully, false if invalid or recording in progress
  bool setFrameRate(uint32_t fps);

  // Set the quality preset (affects resolution, codec, bitrate)
  // @param quality: Quality level
  // @return true if set successfully, false if recording in progress
  bool setQuality(Quality quality);

  // Get current recording statistics
  // @return RecordingStats struct with current values
  RecordingStats getRecordingStats() const;

  // Get the full output path of the current/last recording
  // @return Full path to the video file
  std::string getOutputPath() const;

  // Cancel the current recording and delete the partial file
  // @return true if cancelled successfully, false if not recording
  bool cancelRecording();

private:
  // Prevent copying
  ScreenRecorder(const ScreenRecorder &) = delete;
  ScreenRecorder &operator=(const ScreenRecorder &) = delete;

  // Private implementation details (pimpl idiom for FFmpeg dependencies)
  class Impl;
  Impl *pImpl_;
};

} // namespace cms
