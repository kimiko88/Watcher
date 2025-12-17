#ifndef CMS_MASTER_ACTIVITY_AGGREGATOR_H
#define CMS_MASTER_ACTIVITY_AGGREGATOR_H

#include "cms/ActivityMonitor.h" // Reuse ProcessInfo, ActivityReport
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


namespace cms {

enum class ActivityLevel { LOW, MODERATE, HIGH };

struct ClassroomActivitySummary {
  uint64_t timestamp = 0;
  uint32_t total_students = 0;
  uint32_t active_students = 0;
  float average_cpu_usage = 0.0f;
  float average_ram_usage = 0.0f;
  std::string most_used_app;
  std::string least_idle_student;
  std::string most_idle_student;
  ActivityLevel classroom_activity_level = ActivityLevel::LOW;
};

struct StudentRank {
  uint32_t rank = 0;
  std::string student_name;
  float activity_percentage = 0.0f; // 100 - idle_percentage
  uint32_t apps_count = 0;
  float idle_time_percentage = 0.0f;
  std::string client_id;
};

struct ClientInfo {
  std::string id;
  std::string name;
  // Add other metadata if needed
};

enum class AnomalySeverity { LOW, MEDIUM, HIGH };

struct ClassAnomalyAlert {
  std::string alert_id;
  uint64_t timestamp = 0;
  std::string type; // "mass_idle", "game_usage", "high_load"
  std::vector<std::string> affected_students;
  AnomalySeverity severity = AnomalySeverity::LOW;
};

struct AppStats {
  std::string name;
  uint32_t student_this_count = 0;
  // Could add usage duration later
};

class MasterActivityAggregator {
public:
  MasterActivityAggregator();
  ~MasterActivityAggregator();

  // Core Data Ingestion
  bool addClientActivity(const std::string &client_id,
                         const std::string &student_name,
                         const ActivityReport &report);

  // Queries
  ClassroomActivitySummary getClassroomActivity() const;
  std::optional<ActivityReport>
  getClientActivity(const std::string &client_id) const;

  std::optional<ClientInfo> getMostActiveStudent() const;
  std::optional<ClientInfo> getLeastActiveStudent() const;

  std::vector<StudentRank> getStudentRankingByActivity() const;
  std::vector<AppStats> getTopApplicationsClass() const;

  std::vector<ClassAnomalyAlert> detectClasswideAnomalies() const;

  // Helper for testing mainly, or resetting class
  void clearData();

private:
  mutable std::mutex stats_mutex_;

  struct StudentData {
    std::string name;
    ActivityReport latest_report;
    uint64_t last_update_ts = 0;
    // History could be added here
  };

  std::unordered_map<std::string, StudentData>
      student_data_map_; // client_id -> data

  // Helpers
  ActivityLevel calculateActivityLevel(float avg_cpu, float avg_idle) const;
};

} // namespace cms

#endif // CMS_MASTER_ACTIVITY_AGGREGATOR_H
