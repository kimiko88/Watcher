#ifndef CMS_ACTIVITY_MONITOR_H
#define CMS_ACTIVITY_MONITOR_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>


namespace cms {

struct ProcessInfo {
  uint32_t process_id;
  std::string process_name;
  std::string executable_path;
  std::string window_title;
  float memory_usage_mb;
  float cpu_percentage;
  uint64_t creation_time; // Unix timestamp
  bool is_visible;
};

struct KeyboardActivityStats {
  uint64_t total_keystrokes;
  float keys_per_minute;
  uint64_t idle_time_seconds;
  uint64_t last_activity;
};

struct MouseActivityStats {
  uint64_t total_moves;
  uint64_t total_clicks;
  float clicks_per_minute;
  uint64_t idle_time_seconds;
  uint64_t last_activity;
};

struct NetworkUsageStats {
  uint64_t bytes_sent;
  uint64_t bytes_received;
  float bandwidth_kbps;
  uint32_t active_connections;
};

struct ActivityReport {
  uint64_t timestamp;
  std::vector<ProcessInfo> processes;
  KeyboardActivityStats keyboard_stats;
  MouseActivityStats mouse_stats;
  NetworkUsageStats network_stats;
  float cpu_average;
  float ram_average;
};

class ActivityMonitor {
public:
  virtual ~ActivityMonitor() = default;

  // Factory method to get platform-specific instance
  static std::unique_ptr<ActivityMonitor> Create();

  virtual bool startMonitoring() = 0;
  virtual bool stopMonitoring() = 0;
  virtual bool isMonitoring() const = 0;

  virtual std::vector<ProcessInfo> getActiveApplications() = 0;
  virtual uint64_t getScreenIdleTime() = 0; // ms

  virtual KeyboardActivityStats getKeyboardActivity() = 0;
  virtual MouseActivityStats getMouseActivity() = 0;
  virtual NetworkUsageStats getNetworkUsage() = 0;

  virtual float getCPUUsage() = 0; // 0-100%
  virtual float getRAMUsage() = 0; // 0-100%

  virtual ActivityReport getAllActivity() = 0;
};

} // namespace cms

#endif // CMS_ACTIVITY_MONITOR_H
