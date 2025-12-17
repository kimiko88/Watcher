#include "cms/MasterActivityAggregator.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <vector>


namespace cms {

MasterActivityAggregator::MasterActivityAggregator() = default;
MasterActivityAggregator::~MasterActivityAggregator() = default;

bool MasterActivityAggregator::addClientActivity(
    const std::string &client_id, const std::string &student_name,
    const ActivityReport &report) {
  std::lock_guard<std::mutex> lock(stats_mutex_);

  StudentData &data = student_data_map_[client_id];
  data.name = student_name;
  data.latest_report = report;
  data.last_update_ts = report.timestamp; // uint64_t in ActivityReport

  return true;
}

ClassroomActivitySummary
MasterActivityAggregator::getClassroomActivity() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  ClassroomActivitySummary summary;
  summary.total_students = static_cast<uint32_t>(student_data_map_.size());

  if (summary.total_students == 0)
    return summary;

  float total_cpu = 0.0f;
  float total_ram = 0.0f; // This is average %, not bytes now
  int active_students_count = 0;

  std::string most_active_name;
  uint64_t min_idle_s = UINT64_MAX;

  std::string least_active_name;
  uint64_t max_idle_s = 0;

  std::map<std::string, int> app_counts;

  for (const auto &[id, data] : student_data_map_) {
    const auto &r = data.latest_report;
    total_cpu += r.cpu_average;
    total_ram += r.ram_average;

    // Use keyboard idle time in seconds
    uint64_t idle_s = r.keyboard_stats.idle_time_seconds;

    // Active < 5 mins (300s)
    if (idle_s < 300) {
      active_students_count++;
    }

    if (idle_s < min_idle_s) {
      min_idle_s = idle_s;
      most_active_name = data.name;
    }

    if (idle_s >= max_idle_s) {
      max_idle_s = idle_s;
      least_active_name = data.name;
    }

    for (const auto &proc : r.processes) {
      app_counts[proc.process_name]++;
    }
  }

  summary.active_students = active_students_count;
  summary.average_cpu_usage = total_cpu / summary.total_students;
  summary.average_ram_usage =
      total_ram / summary.total_students; // This is % now
  summary.least_idle_student = most_active_name;
  summary.most_idle_student = least_active_name;

  std::string top_app;
  int max_count = 0;
  for (const auto &[sf, count] : app_counts) {
    if (count > max_count) {
      max_count = count;
      top_app = sf;
    }
  }
  summary.most_used_app = top_app;

  float active_ratio = (float)active_students_count / summary.total_students;
  if (active_ratio > 0.8f)
    summary.classroom_activity_level = ActivityLevel::HIGH;
  else if (active_ratio > 0.5f)
    summary.classroom_activity_level = ActivityLevel::MODERATE;
  else
    summary.classroom_activity_level = ActivityLevel::LOW;

  return summary;
}

std::optional<ActivityReport> MasterActivityAggregator::getClientActivity(
    const std::string &client_id) const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  auto it = student_data_map_.find(client_id);
  if (it != student_data_map_.end()) {
    return it->second.latest_report;
  }
  return std::nullopt;
}

std::optional<ClientInfo>
MasterActivityAggregator::getMostActiveStudent() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  if (student_data_map_.empty())
    return std::nullopt;

  std::string best_id;
  uint64_t min_idle = UINT64_MAX;
  std::string name;

  for (const auto &[id, data] : student_data_map_) {
    uint64_t idle = data.latest_report.keyboard_stats.idle_time_seconds;
    if (idle < min_idle) {
      min_idle = idle;
      best_id = id;
      name = data.name;
    }
  }
  return ClientInfo{best_id, name};
}

std::optional<ClientInfo>
MasterActivityAggregator::getLeastActiveStudent() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  if (student_data_map_.empty())
    return std::nullopt;

  std::string worst_id;
  uint64_t max_idle = 0;

  auto it = student_data_map_.begin();
  worst_id = it->first;
  max_idle = it->second.latest_report.keyboard_stats.idle_time_seconds;
  std::string name = it->second.name;
  it++;

  for (; it != student_data_map_.end(); ++it) {
    uint64_t idle = it->second.latest_report.keyboard_stats.idle_time_seconds;
    if (idle > max_idle) {
      max_idle = idle;
      worst_id = it->first;
      name = it->second.name;
    }
  }
  return ClientInfo{worst_id, name};
}

std::vector<StudentRank>
MasterActivityAggregator::getStudentRankingByActivity() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  std::vector<StudentRank> ranks;
  ranks.reserve(student_data_map_.size());

  for (const auto &[id, data] : student_data_map_) {
    StudentRank r;
    r.student_name = data.name;
    r.client_id = id;
    r.apps_count = static_cast<uint32_t>(data.latest_report.processes.size());

    float idle_ratio =
        (float)data.latest_report.keyboard_stats.idle_time_seconds / 300.0f;
    if (idle_ratio > 1.0f)
      idle_ratio = 1.0f;
    r.idle_time_percentage = idle_ratio * 100.0f;
    r.activity_percentage = 100.0f - r.idle_time_percentage;

    ranks.push_back(r);
  }

  std::sort(ranks.begin(), ranks.end(),
            [](const StudentRank &a, const StudentRank &b) {
              return a.activity_percentage > b.activity_percentage;
            });

  for (size_t i = 0; i < ranks.size(); ++i) {
    ranks[i].rank = static_cast<uint32_t>(i + 1);
  }

  return ranks;
}

std::vector<AppStats>
MasterActivityAggregator::getTopApplicationsClass() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  std::map<std::string, int> counts;
  for (const auto &[id, data] : student_data_map_) {
    std::map<std::string, bool> student_apps;
    for (const auto &p : data.latest_report.processes) {
      student_apps[p.process_name] = true;
    }
    for (const auto &[app, _] : student_apps) {
      counts[app]++;
    }
  }

  std::vector<AppStats> result;
  for (const auto &[name, count] : counts) {
    result.push_back({name, (uint32_t)count});
  }

  std::sort(result.begin(), result.end(),
            [](const AppStats &a, const AppStats &b) {
              return a.student_this_count > b.student_this_count;
            });

  return result;
}

std::vector<ClassAnomalyAlert>
MasterActivityAggregator::detectClasswideAnomalies() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  std::vector<ClassAnomalyAlert> alerts;

  if (student_data_map_.empty())
    return alerts;

  int idle_count = 0;
  std::vector<std::string> idle_students;
  uint64_t IDLE_THRESHOLD_S = 60; // 60s

  std::vector<std::string> gamers;
  std::vector<std::string> games = {"fortnite", "minecraft", "league", "roblox",
                                    "steam"};

  std::vector<std::string> miners;

  for (const auto &[id, data] : student_data_map_) {
    if (data.latest_report.keyboard_stats.idle_time_seconds >
        IDLE_THRESHOLD_S) {
      idle_count++;
      idle_students.push_back(data.name);
    }

    if (data.latest_report.cpu_average > 90.0f) {
      miners.push_back(data.name);
    }

    for (const auto &p : data.latest_report.processes) {
      std::string pname = p.process_name;
      std::transform(pname.begin(), pname.end(), pname.begin(), ::tolower);
      for (const auto &g : games) {
        if (pname.find(g) != std::string::npos) {
          gamers.push_back(data.name);
          break;
        }
      }
    }
  }

  if ((float)idle_count / student_data_map_.size() >= 0.5f &&
      student_data_map_.size() > 1) {
    ClassAnomalyAlert alert;
    alert.alert_id =
        "mass_idle_" +
        std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
    alert.type = "mass_idle";
    alert.affected_students = idle_students;
    alert.severity = AnomalySeverity::MEDIUM;
    alerts.push_back(alert);
  }

  if (!gamers.empty()) {
    ClassAnomalyAlert alert;
    alert.alert_id =
        "game_" +
        std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
    alert.type = "game_usage";
    alert.affected_students = gamers;
    alert.severity = AnomalySeverity::HIGH;
    alerts.push_back(alert);
  }

  if (!miners.empty()) {
    ClassAnomalyAlert alert;
    alert.alert_id =
        "load_" +
        std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
    alert.type = "high_load";
    alert.affected_students = miners;
    alert.severity = AnomalySeverity::MEDIUM;
    alerts.push_back(alert);
  }

  return alerts;
}

void MasterActivityAggregator::clearData() {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  student_data_map_.clear();
}

ActivityLevel
MasterActivityAggregator::calculateActivityLevel(float avg_cpu,
                                                 float avg_idle) const {
  return ActivityLevel::LOW;
}

} // namespace cms
