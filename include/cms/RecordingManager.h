#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cms {

// Information about a single recording
struct RecordingInfo {
  std::string recording_id; // UUID
  std::string filename;
  std::string client_id;    // Empty if master recording
  std::string student_name; // Optional
  uint64_t timestamp;       // Unix timestamp
  uint64_t duration_seconds;
  float file_size_mb;
  std::string codec;
  std::string resolution;
  std::vector<std::string> tags;
  std::string notes;
};

// Storage statistics
struct RecordingStorageStats {
  uint32_t total_recordings = 0;
  float total_storage_mb = 0.0f;
  float quota_mb = 0.0f;
  float usage_percentage = 0.0f;
};

// Manager for recorded sessions
class RecordingManager {
public:
  // Constructor
  // @param recordings_directory: Directory where recordings are stored
  explicit RecordingManager(const std::string &recordings_directory);

  // Destructor
  ~RecordingManager();

  // List all available recordings
  // @return Vector of recording metadata
  std::vector<RecordingInfo> listRecordings() const;

  // Get detailed information about a specific recording
  // @param recording_id: UUID of the recording
  // @return RecordingInfo if found, empty optional otherwise
  RecordingInfo getRecordingInfo(const std::string &recording_id) const;

  // Play a recording (launches default video player or returns path)
  // @param recording_id: UUID of the recording
  // @return true if playback started successfully
  bool playRecording(const std::string &recording_id);

  // Delete a recording permanently
  // @param recording_id: UUID of the recording
  // @return true if deleted successfully
  bool deleteRecording(const std::string &recording_id);

  // Export recording to a different format
  // @param recording_id: UUID of the recording
  // @param format: Target format (e.g., "mp4", "webm", "avi")
  // @return true if export successful
  bool exportRecording(const std::string &recording_id,
                       const std::string &format);

  // Get total storage used by all recordings
  // @return Storage in bytes
  uint64_t getTotalStorageUsed() const;

  // Get storage quota limit
  // @return Quota in bytes (0 = unlimited)
  uint64_t getStorageQuota() const;

  // Set storage quota
  // @param quota_bytes: Maximum allowed storage
  void setStorageQuota(uint64_t quota_bytes);

  // Clean up recordings older than specified days
  // @param days: Age threshold in days
  // @return Number of recordings deleted
  uint32_t cleanupOldRecordings(uint32_t days);

  // Get storage statistics
  // @return RecordingStorageStats struct
  RecordingStorageStats getRecordingStats() const;

  // Add tags to a recording
  // @param recording_id: UUID of the recording
  // @param tags: Tags to add
  // @return true if successful
  bool addTags(const std::string &recording_id,
               const std::vector<std::string> &tags);

  // Update recording notes
  // @param recording_id: UUID of the recording
  // @param notes: Notes text
  // @return true if successful
  bool updateNotes(const std::string &recording_id, const std::string &notes);

  // Search recordings by tag
  // @param tag: Tag to search for
  // @return Vector of matching recordings
  std::vector<RecordingInfo> searchByTag(const std::string &tag) const;

private:
  // Prevent copying
  RecordingManager(const RecordingManager &) = delete;
  RecordingManager &operator=(const RecordingManager &) = delete;

  // Private implementation (pimpl)
  class Impl;
  Impl *pImpl_;
};

} // namespace cms
