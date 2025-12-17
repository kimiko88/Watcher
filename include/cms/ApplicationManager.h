#ifndef CMS_APPLICATION_MANAGER_H
#define CMS_APPLICATION_MANAGER_H

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace cms {

enum class AppFilterMode { MODE_DISABLED, MODE_BLACKLIST, MODE_WHITELIST };

enum class RuleAction { BLOCK, ALLOW };

struct ApplicationRule {
  std::string rule_id; // UUID usually
  std::string app_path;
  std::string app_name;
  std::string process_pattern; // regex
  RuleAction action;
  bool enabled;
  uint64_t created_at; // timestamp

  // Equality for testing
  bool operator==(const ApplicationRule &other) const {
    return rule_id == other.rule_id;
  }
};

struct BlockedAppStat {
  std::string application_name;
  uint64_t times_blocked = 0;
  uint64_t last_blocked = 0;    // timestamp
  uint64_t bytes_prevented = 0; // approximate?
};

class ApplicationManager {
public:
  ApplicationManager() = default;

  // Virtual destructor if we had inheritance, but here we just impl methods.
  ~ApplicationManager() = default;

  // Rule Management - INLINE IMPL
  bool addToBlacklist(const std::string &app_path,
                      const std::string &app_name) {
    ApplicationRule rule;
    rule.rule_id = app_path; // Simple ID for now
    rule.app_path = app_path;
    rule.app_name = app_name;
    rule.action = RuleAction::BLOCK;
    rule.enabled = true;
    rule.created_at = 0; // clock?
    addRule(rule);
    return true;
  }

  bool removeFromBlacklist(const std::string &app_path) {
    std::lock_guard<std::mutex> lock(rules_mutex_);
    auto it = std::remove_if(
        rules_.begin(), rules_.end(), [&](const ApplicationRule &r) {
          return r.app_path == app_path && r.action == RuleAction::BLOCK;
        });
    bool removed = (it != rules_.end());
    rules_.erase(it, rules_.end());
    return removed;
  }

  bool addToWhitelist(const std::string &app_path,
                      const std::string &app_name) {
    ApplicationRule rule;
    rule.rule_id = app_path;
    rule.app_path = app_path;
    rule.app_name = app_name;
    rule.action = RuleAction::ALLOW;
    rule.enabled = true;
    addRule(rule);
    return true;
  }

  bool removeFromWhitelist(const std::string &app_path) {
    std::lock_guard<std::mutex> lock(rules_mutex_);
    auto it = std::remove_if(
        rules_.begin(), rules_.end(), [&](const ApplicationRule &r) {
          return r.app_path == app_path && r.action == RuleAction::ALLOW;
        });
    bool removed = (it != rules_.end());
    rules_.erase(it, rules_.end());
    return removed;
  }

  bool setMode(AppFilterMode mode) {
    std::lock_guard<std::mutex> lock(rules_mutex_);
    current_mode_ = mode;
    return true;
  }

  AppFilterMode getMode() const {
    std::lock_guard<std::mutex> lock(rules_mutex_);
    return current_mode_;
  }

  void addRule(const ApplicationRule &rule) {
    std::lock_guard<std::mutex> lock(rules_mutex_);
    // Simple dedupe by ID or content? For now append.
    rules_.push_back(rule);
  }

  // Core Logic - INLINE IMPL (using helpers)
  bool isApplicationAllowed(uint32_t process_id) const {
    std::string path = resolvePathFromPid(process_id);
    if (path.empty())
      return true; // Can't resolve? Fail open or closed? Open for safety.
    return isApplicationAllowed(path);
  }

  bool isApplicationAllowed(const std::string &app_path) const {
    std::lock_guard<std::mutex> lock(rules_mutex_);
    if (current_mode_ == AppFilterMode::MODE_DISABLED)
      return true;

    if (current_mode_ == AppFilterMode::MODE_BLACKLIST) {
      // Default Allow. Block if ANY rule strictly matches BLOCK.
      for (const auto &rule : rules_) {
        if (rule.enabled && rule.action == RuleAction::BLOCK) {
          if (matchesRule(app_path, rule))
            return false;
        }
      }
      return true;
    }

    if (current_mode_ == AppFilterMode::MODE_WHITELIST) {
      // Default Block. Allow if ANY rule strictly matches ALLOW.
      for (const auto &rule : rules_) {
        if (rule.enabled && rule.action == RuleAction::ALLOW) {
          if (matchesRule(app_path, rule))
            return true;
        }
      }
      return false;
    }
    return true;
  }

  // Actions - Platform Specific / Implemented in CPP
  bool blockRunningApplication(uint32_t process_id);
  bool terminateApplication(uint32_t process_id);

  // Stats - INLINE
  std::vector<BlockedAppStat> getBlockedApplicationStats() const {
    std::lock_guard<std::mutex> lock(rules_mutex_);
    std::vector<BlockedAppStat> s;
    for (const auto &kv : stats_) {
      s.push_back(kv.second);
    }
    return s;
  }

  // Persistence - Implemented in CPP
  bool exportRules(const std::string &filepath);
  bool importRules(const std::string &filepath);

private:
  mutable std::mutex rules_mutex_;
  AppFilterMode current_mode_ = AppFilterMode::MODE_DISABLED;
  std::vector<ApplicationRule> rules_;
  std::unordered_map<std::string, BlockedAppStat> stats_;

  // Helpers
  std::string
  resolvePathFromPid(uint32_t process_id) const; // Platform specific

  bool matchesRule(const std::string &path, const ApplicationRule &rule) const {
    // 1. Literal Path Match (Case Insensitive for Windows, ideally generic
    // logic handles platform diffs) I'll use simple string find for now.
    // 2. Regex
    if (!rule.process_pattern.empty()) {
      try {
        std::regex re(rule.process_pattern,
                      std::regex_constants::icase); // Case insensitive default
                                                    // often good for apps
        if (std::regex_match(path, re))
          return true;
        // Try matching just filename too?
        // Path usually full path.
      } catch (...) {
        // Ignore invalid regex
      }
    }

    // Simple case insensitive equality
    std::string p1 = path;
    std::string p2 = rule.app_path;
    if (p2.empty() && rule.process_pattern.empty())
      return false; // Empty rule matches nothing?
    if (p2.empty())
      return false; // If only regex, we checked it.

    std::transform(p1.begin(), p1.end(), p1.begin(), ::tolower);
    std::transform(p2.begin(), p2.end(), p2.begin(), ::tolower);
    return p1 == p2;
  }
};

} // namespace cms

#endif // CMS_APPLICATION_MANAGER_H
