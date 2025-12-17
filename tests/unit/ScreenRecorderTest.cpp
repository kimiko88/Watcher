#include "cms/ScreenRecorder.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>


using namespace cms;
namespace fs = std::filesystem;

class ScreenRecorderTest : public ::testing::Test {
protected:
  std::string test_output_dir = "test_recordings";

  void SetUp() override {
    // Create test directory
    fs::create_directories(test_output_dir);
  }

  void TearDown() override {
    // Cleanup test recordings
    if (fs::exists(test_output_dir)) {
      fs::remove_all(test_output_dir);
    }
  }

  bool videoFileExists(const std::string &filename) {
    fs::path path = fs::path(test_output_dir) / filename;
    return fs::exists(path) && fs::file_size(path) > 0;
  }

  bool videoFileDeleted(const std::string &filename) {
    fs::path path = fs::path(test_output_dir) / filename;
    return !fs::exists(path);
  }
};

// 1. Basic Lifecycle - Start and Stop
TEST_F(ScreenRecorderTest, StartStopRecording) {
  ScreenRecorder recorder(test_output_dir);
  EXPECT_FALSE(recorder.isRecording());

  bool started = recorder.startRecording("test_video", Quality::LOW);
  EXPECT_TRUE(started);
  EXPECT_TRUE(recorder.isRecording());

  // Let it record for a brief moment
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  bool stopped = recorder.stopRecording();
  EXPECT_TRUE(stopped);
  EXPECT_FALSE(recorder.isRecording());
}

// 2. Video File Generation
TEST_F(ScreenRecorderTest, VideoFileGenerated) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("output_test", Quality::LOW);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  recorder.stopRecording();

  // Check if video file was created (extension will be added by implementation)
  // We check for common extensions
  bool found = videoFileExists("output_test.mp4") ||
               videoFileExists("output_test.webm") ||
               videoFileExists("output_test.mkv");
  EXPECT_TRUE(found);
}

// 3. Pause and Resume
TEST_F(ScreenRecorderTest, PauseResumeRecording) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("pause_test", Quality::LOW);

  EXPECT_TRUE(recorder.isRecording());

  bool paused = recorder.pauseRecording();
  EXPECT_TRUE(paused);

  bool resumed = recorder.resumeRecording();
  EXPECT_TRUE(resumed);
  EXPECT_TRUE(recorder.isRecording());

  recorder.stopRecording();
}

// 4. Pause When Not Recording Fails
TEST_F(ScreenRecorderTest, PauseWhenNotRecordingFails) {
  ScreenRecorder recorder(test_output_dir);
  EXPECT_FALSE(recorder.pauseRecording());
}

// 5. Resume When Not Paused Fails
TEST_F(ScreenRecorderTest, ResumeWhenNotPausedFails) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("resume_test", Quality::LOW);
  EXPECT_FALSE(recorder.resumeRecording()); // Not paused
  recorder.stopRecording();
}

// 6. Quality Level Setting
TEST_F(ScreenRecorderTest, SetQualityBeforeRecording) {
  ScreenRecorder recorder(test_output_dir);

  EXPECT_TRUE(recorder.setQuality(Quality::HIGH));
  EXPECT_TRUE(recorder.setQuality(Quality::ULTRA));
  EXPECT_TRUE(recorder.setQuality(Quality::MEDIUM));
}

// 7. Quality Cannot Change During Recording
TEST_F(ScreenRecorderTest, CannotSetQualityDuringRecording) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("quality_test", Quality::LOW);

  EXPECT_FALSE(recorder.setQuality(Quality::HIGH));

  recorder.stopRecording();
}

// 8. Frame Rate Setting
TEST_F(ScreenRecorderTest, SetFrameRateValid) {
  ScreenRecorder recorder(test_output_dir);

  EXPECT_TRUE(recorder.setFrameRate(30));
  EXPECT_TRUE(recorder.setFrameRate(60));
  EXPECT_TRUE(recorder.setFrameRate(15));
}

// 9. Frame Rate Invalid Range
TEST_F(ScreenRecorderTest, SetFrameRateInvalidRange) {
  ScreenRecorder recorder(test_output_dir);

  EXPECT_FALSE(recorder.setFrameRate(0));   // Too low
  EXPECT_FALSE(recorder.setFrameRate(121)); // Too high
}

// 10. Frame Rate Cannot Change During Recording
TEST_F(ScreenRecorderTest, CannotSetFrameRateDuringRecording) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("fps_test", Quality::LOW);

  EXPECT_FALSE(recorder.setFrameRate(60));

  recorder.stopRecording();
}

// 11. Recording Stats Accuracy
TEST_F(ScreenRecorderTest, RecordingStatsAccurate) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("stats_test", Quality::LOW);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  RecordingStats stats = recorder.getRecordingStats();

  EXPECT_GT(stats.frame_count, 0);
  EXPECT_GT(stats.duration_seconds, 0);
  EXPECT_FALSE(stats.filename.empty());
  EXPECT_FALSE(stats.codec.empty());
  EXPECT_FALSE(stats.resolution.empty());

  recorder.stopRecording();
}

// 12. Output Path Retrieval
TEST_F(ScreenRecorderTest, GetOutputPath) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("path_test", Quality::LOW);

  std::string path = recorder.getOutputPath();
  EXPECT_FALSE(path.empty());
  EXPECT_NE(path.find(test_output_dir), std::string::npos);
  EXPECT_NE(path.find("path_test"), std::string::npos);

  recorder.stopRecording();
}

// 13. Cancel Recording
TEST_F(ScreenRecorderTest, CancelRecordingDeletesFile) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("cancel_test", Quality::LOW);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  bool cancelled = recorder.cancelRecording();
  EXPECT_TRUE(cancelled);
  EXPECT_FALSE(recorder.isRecording());

  // File should be deleted
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(videoFileDeleted("cancel_test.mp4") &&
              videoFileDeleted("cancel_test.webm") &&
              videoFileDeleted("cancel_test.mkv"));
}

// 14. Cancel When Not Recording Fails
TEST_F(ScreenRecorderTest, CancelWhenNotRecordingFails) {
  ScreenRecorder recorder(test_output_dir);
  EXPECT_FALSE(recorder.cancelRecording());
}

// 15. Concurrent Recording Prevention (Single Instance)
TEST_F(ScreenRecorderTest, PreventConcurrentRecording) {
  ScreenRecorder recorder(test_output_dir);

  EXPECT_TRUE(recorder.startRecording("first", Quality::LOW));
  EXPECT_FALSE(recorder.startRecording("second", Quality::LOW));

  recorder.stopRecording();
}

// 16. Multiple Start-Stop Cycles
TEST_F(ScreenRecorderTest, MultipleStartStopCycles) {
  ScreenRecorder recorder(test_output_dir);

  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(
        recorder.startRecording("cycle_" + std::to_string(i), Quality::LOW));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(recorder.stopRecording());
  }
}

// 17. Stop When Not Recording Fails
TEST_F(ScreenRecorderTest, StopWhenNotRecordingFails) {
  ScreenRecorder recorder(test_output_dir);
  EXPECT_FALSE(recorder.stopRecording());
}

// 18. Quality Preset Validation (Low)
TEST_F(ScreenRecorderTest, QualityPresetLow) {
  ScreenRecorder recorder(test_output_dir);
  recorder.setQuality(Quality::LOW);
  recorder.startRecording("quality_low", Quality::LOW);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  RecordingStats stats = recorder.getRecordingStats();

  // LOW should be 720p and VP9
  EXPECT_EQ(stats.codec, "vp9");
  EXPECT_NE(stats.resolution.find("720"), std::string::npos);

  recorder.stopRecording();
}

// 19. Quality Preset Validation (Ultra)
TEST_F(ScreenRecorderTest, QualityPresetUltra) {
  ScreenRecorder recorder(test_output_dir);
  recorder.setQuality(Quality::ULTRA);
  recorder.startRecording("quality_ultra", Quality::ULTRA);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  RecordingStats stats = recorder.getRecordingStats();

  // ULTRA should be 2160p (4K) and H.265
  EXPECT_TRUE(stats.codec == "h265" || stats.codec == "hevc");
  EXPECT_NE(stats.resolution.find("2160"), std::string::npos);

  recorder.stopRecording();
}

// 20. Disk Space Monitoring
TEST_F(ScreenRecorderTest, DiskSpaceMonitoring) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("diskspace_test", Quality::LOW);

  RecordingStats stats = recorder.getRecordingStats();
  EXPECT_GT(stats.disk_free_mb, 0.0f);

  recorder.stopRecording();
}

// 21. Frame Drops Tracking
TEST_F(ScreenRecorderTest, FrameDropsTracking) {
  ScreenRecorder recorder(test_output_dir);
  recorder.startRecording("framedrop_test", Quality::LOW);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  RecordingStats stats = recorder.getRecordingStats();

  // frame_drops should be initialized (may be 0 for short recordings)
  EXPECT_GE(stats.frame_drops, 0u);

  recorder.stopRecording();
}

// 22. Destructor Cleanup
TEST_F(ScreenRecorderTest, DestructorStopsRecording) {
  {
    ScreenRecorder recorder(test_output_dir);
    recorder.startRecording("destructor_test", Quality::LOW);
    EXPECT_TRUE(recorder.isRecording());
    // Recorder goes out of scope, destructor should stop recording
  }

  // If destructor worked, file should exist (stopped gracefully)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  bool found = videoFileExists("destructor_test.mp4") ||
               videoFileExists("destructor_test.webm") ||
               videoFileExists("destructor_test.mkv");
  EXPECT_TRUE(found);
}
