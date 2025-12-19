#include "cms/RecordingManager.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>

namespace cms {
namespace fs = std::filesystem;
using json = nlohmann::json;

// Helper to generate UUID (simple implementation)
#include <atomic>

// Helper to generate UUID (simple implementation)
static std::string generateUUID() {
  static std::atomic<int> counter{0};
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch())
                       .count();
  return "rec_" + std::to_string(timestamp) + "_" + std::to_string(counter++);
}

// Internal implementation
class RecordingManager::Impl {
public:
  std::string recordings_dir;
  std::string metadata_file;
  uint64_t storage_quota = 0; // 0 = unlimited
  mutable std::mutex mutex;

  std::vector<RecordingInfo> recordings_cache;

  explicit Impl(const std::string &dir)
      : recordings_dir(dir), metadata_file(dir + "/recordings_metadata.json") {

    fs::create_directories(recordings_dir);
    loadMetadata();
    scanForNewRecordings();
  }

  void loadMetadata() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!fs::exists(metadata_file)) {
      return; // No metadata yet
    }

    try {
      std::ifstream file(metadata_file);
      json data = json::parse(file);

      for (const auto &item : data["recordings"]) {
        RecordingInfo info;
        info.recording_id = item.value("id", "");
        info.filename = item.value("filename", "");
        info.client_id = item.value("client_id", "");
        info.student_name = item.value("student_name", "");
        info.timestamp = item.value("timestamp", 0);
        info.duration_seconds = item.value("duration", 0);
        info.file_size_mb = item.value("file_size_mb", 0.0f);
        info.codec = item.value("codec", "");
        info.resolution = item.value("resolution", "");
        info.notes = item.value("notes", "");

        if (item.contains("tags")) {
          for (const auto &tag : item["tags"]) {
            info.tags.push_back(tag.get<std::string>());
          }
        }

        recordings_cache.push_back(info);
      }
    } catch (...) {
      // Ignore parse errors
    }
  }

  void saveMetadata() {
    std::lock_guard<std::mutex> lock(mutex);

    json data;
    data["recordings"] = json::array();

    for (const auto &rec : recordings_cache) {
      json item;
      item["id"] = rec.recording_id;
      item["filename"] = rec.filename;
      item["client_id"] = rec.client_id;
      item["student_name"] = rec.student_name;
      item["timestamp"] = rec.timestamp;
      item["duration"] = rec.duration_seconds;
      item["file_size_mb"] = rec.file_size_mb;
      item["codec"] = rec.codec;
      item["resolution"] = rec.resolution;
      item["notes"] = rec.notes;
      item["tags"] = rec.tags;

      data["recordings"].push_back(item);
    }

    std::ofstream file(metadata_file);
    file << data.dump(2);
  }

  void scanForNewRecordings() {
    // Scan directory for video files not in metadata
    if (!fs::exists(recordings_dir))
      return;

    for (const auto &entry : fs::directory_iterator(recordings_dir)) {
      if (!entry.is_regular_file())
        continue;

      std::string ext = entry.path().extension().string();
      if (ext != ".mp4" && ext != ".webm" && ext != ".mkv")
        continue;

      std::string filename = entry.path().filename().string();

      // Check if already in cache
      bool found = false;
      for (const auto &rec : recordings_cache) {
        if (rec.filename == filename) {
          found = true;
          break;
        }
      }

      if (!found) {
        // Add new recording
        RecordingInfo info;
        info.recording_id = generateUUID();
        info.filename = filename;
        info.timestamp = fs::last_write_time(entry).time_since_epoch().count();
        info.file_size_mb =
            static_cast<float>(entry.file_size()) / (1024.0f * 1024.0f);
        info.codec = "unknown";
        info.resolution = "unknown";

        recordings_cache.push_back(info);
      }
    }

    saveMetadata();
  }

  std::vector<RecordingInfo> listRecordings() const {
    std::lock_guard<std::mutex> lock(mutex);
    return recordings_cache;
  }

  RecordingInfo getRecordingInfo(const std::string &id) const {
    std::lock_guard<std::mutex> lock(mutex);

    for (const auto &rec : recordings_cache) {
      if (rec.recording_id == id) {
        return rec;
      }
    }

    return RecordingInfo(); // Empty if not found
  }

  bool deleteRecording(const std::string &id) {
    {
      std::lock_guard<std::mutex> lock(mutex);

      auto it = std::find_if(
          recordings_cache.begin(), recordings_cache.end(),
          [&id](const RecordingInfo &r) { return r.recording_id == id; });

      if (it == recordings_cache.end()) {
        return false;
      }

      // Delete file
      fs::path file_path = fs::path(recordings_dir) / it->filename;
      if (fs::exists(file_path)) {
        std::error_code ec;
        if (!fs::remove(file_path, ec)) {
          // Log error if we had a logger here, or just print to stderr for test
          // debugging
          std::cerr << "Failed to delete file: " << file_path
                    << " Error: " << ec.message() << std::endl;
        }
      }

      // Remove from cache
      recordings_cache.erase(it);
    } // Release lock before calling saveMetadata

    saveMetadata();
    return true;
  }

  uint64_t getTotalStorageUsed() const {
    std::lock_guard<std::mutex> lock(mutex);

    uint64_t total = 0;
    for (const auto &rec : recordings_cache) {
      total += static_cast<uint64_t>(rec.file_size_mb * 1024 * 1024);
    }
    return total;
  }

  RecordingStorageStats getStats() const {
    std::lock_guard<std::mutex> lock(mutex);

    RecordingStorageStats stats;
    stats.total_recordings = static_cast<uint32_t>(recordings_cache.size());

    // Calculate total storage directly to avoid mutex deadlock
    uint64_t total_bytes = 0;
    for (const auto &rec : recordings_cache) {
      total_bytes += static_cast<uint64_t>(rec.file_size_mb * 1024 * 1024);
    }
    stats.total_storage_mb =
        static_cast<float>(total_bytes) / (1024.0f * 1024.0f);
    stats.quota_mb = static_cast<float>(storage_quota) / (1024.0f * 1024.0f);

    if (storage_quota > 0) {
      stats.usage_percentage =
          (stats.total_storage_mb / stats.quota_mb) * 100.0f;
    } else {
      stats.usage_percentage = 0.0f;
    }

    return stats;
  }

  uint32_t cleanupOldRecordings(uint32_t days) {
    uint32_t deleted = 0;

    {
      std::lock_guard<std::mutex> lock(mutex);

      auto now = std::chrono::system_clock::now();
      auto threshold = now - std::chrono::hours(24 * days);
      auto threshold_ts = std::chrono::duration_cast<std::chrono::seconds>(
                              threshold.time_since_epoch())
                              .count();

      auto it = recordings_cache.begin();
      while (it != recordings_cache.end()) {
        if (it->timestamp < static_cast<uint64_t>(threshold_ts)) {
          // Delete file
          fs::path file_path = fs::path(recordings_dir) / it->filename;
          if (fs::exists(file_path)) {
            fs::remove(file_path);
          }

          it = recordings_cache.erase(it);
          deleted++;
        } else {
          ++it;
        }
      }
    } // Release lock

    if (deleted > 0) {
      saveMetadata();
    }

    return deleted;
  }

  bool addTags(const std::string &id, const std::vector<std::string> &tags) {
    bool found = false;
    {
      std::lock_guard<std::mutex> lock(mutex);

      for (auto &rec : recordings_cache) {
        if (rec.recording_id == id) {
          for (const auto &tag : tags) {
            if (std::find(rec.tags.begin(), rec.tags.end(), tag) ==
                rec.tags.end()) {
              rec.tags.push_back(tag);
            }
          }
          found = true;
          break; // Found and updated, exit loop
        }
      }
    } // Release lock

    if (found) {
      saveMetadata();
    }

    return found;
  }

  bool updateNotes(const std::string &id, const std::string &notes) {
    bool found = false;
    {
      std::lock_guard<std::mutex> lock(mutex);

      for (auto &rec : recordings_cache) {
        if (rec.recording_id == id) {
          rec.notes = notes;
          found = true;
          break; // Found and updated, exit loop
        }
      }
    } // Release lock

    if (found) {
      saveMetadata();
    }

    return found;
  }

  std::vector<RecordingInfo> searchByTag(const std::string &tag) const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<RecordingInfo> results;

    for (const auto &rec : recordings_cache) {
      if (std::find(rec.tags.begin(), rec.tags.end(), tag) != rec.tags.end()) {
        results.push_back(rec);
      }
    }

    return results;
  }

  bool playRecording(const std::string &id) {
    auto info = getRecordingInfo(id);
    if (info.recording_id.empty()) {
      return false;
    }

    fs::path file_path = fs::path(recordings_dir) / info.filename;
    if (!fs::exists(file_path)) {
      return false;
    }

    // Stub: In real implementation, would launch video player
    // For now, just verify file exists
    return true;
  }

  bool exportRecording(const std::string &id, const std::string &format) {
    auto info = getRecordingInfo(id);
    if (info.recording_id.empty()) {
      return false;
    }

    // Validate format
    if (format != "mp4" && format != "webm" && format != "avi") {
      return false; // Unsupported format
    }

    // Stub: In real implementation, would use FFmpeg to convert
    return true;
  }
};

// Public interface implementation
RecordingManager::RecordingManager(const std::string &recordings_directory)
    : pImpl_(new Impl(recordings_directory)) {}

RecordingManager::~RecordingManager() { delete pImpl_; }

std::vector<RecordingInfo> RecordingManager::listRecordings() const {
  return pImpl_->listRecordings();
}

RecordingInfo
RecordingManager::getRecordingInfo(const std::string &recording_id) const {
  return pImpl_->getRecordingInfo(recording_id);
}

bool RecordingManager::playRecording(const std::string &recording_id) {
  return pImpl_->playRecording(recording_id);
}

bool RecordingManager::deleteRecording(const std::string &recording_id) {
  return pImpl_->deleteRecording(recording_id);
}

bool RecordingManager::exportRecording(const std::string &recording_id,
                                       const std::string &format) {
  return pImpl_->exportRecording(recording_id, format);
}

uint64_t RecordingManager::getTotalStorageUsed() const {
  return pImpl_->getTotalStorageUsed();
}

uint64_t RecordingManager::getStorageQuota() const {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);
  return pImpl_->storage_quota;
}

void RecordingManager::setStorageQuota(uint64_t quota_bytes) {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);
  pImpl_->storage_quota = quota_bytes;
}

uint32_t RecordingManager::cleanupOldRecordings(uint32_t days) {
  return pImpl_->cleanupOldRecordings(days);
}

RecordingStorageStats RecordingManager::getRecordingStats() const {
  return pImpl_->getStats();
}

bool RecordingManager::addTags(const std::string &recording_id,
                               const std::vector<std::string> &tags) {
  return pImpl_->addTags(recording_id, tags);
}

bool RecordingManager::updateNotes(const std::string &recording_id,
                                   const std::string &notes) {
  return pImpl_->updateNotes(recording_id, notes);
}

std::vector<RecordingInfo>
RecordingManager::searchByTag(const std::string &tag) const {
  return pImpl_->searchByTag(tag);
}

} // namespace cms
