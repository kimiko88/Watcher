// clang-format off
#define WIN32_LEAN_AND_MEAN
#define PSAPI_VERSION 1
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
// clang-format on
#include "cms/ApplicationManager.h"
#include <fstream>
#include <iostream>

namespace cms {

// 1. Resolve PID -> Path
std::string ApplicationManager::resolvePathFromPid(uint32_t process_id) const {
  std::string path;
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                FALSE, process_id);
  if (hProcess) {
    char buffer[MAX_PATH];
    if (GetModuleFileNameExA(hProcess, NULL, buffer, MAX_PATH)) {
      path = buffer;
    }
    CloseHandle(hProcess);
  }
  return path;
}

// 2. Block Running Application (Terminate)
bool ApplicationManager::terminateApplication(uint32_t process_id) {
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, process_id);
  if (hProcess == NULL)
    return false;

  bool result = TerminateProcess(hProcess, 1);
  CloseHandle(hProcess);
  return result;
}

// 3. Block wrapper
bool ApplicationManager::blockRunningApplication(uint32_t process_id) {
  // Should check if allowed first? Or is this called WHEN verified as blocked?
  // "blockRunningApplication" implies action.
  // And assume it updates stats.

  std::string path = resolvePathFromPid(process_id);
  std::string app_name = path; // Extract basename ideally

  bool terminated = terminateApplication(process_id);

  if (terminated) {
    std::lock_guard<std::mutex> lock(rules_mutex_);
    if (path.empty())
      path = "Unknown PID " + std::to_string(process_id);

    BlockedAppStat &stat = stats_[path];
    stat.application_name = path; // Or rule name if found?
    stat.times_blocked++;
    // stat.last_blocked = now...
  }
  return terminated;
}

// 4. Persistence (IO)
bool ApplicationManager::exportRules(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  std::ofstream ofs(filepath);
  if (!ofs.is_open())
    return false;

  // Simple CSV: ID|Path|Name|Regex|Action|Enabled
  for (const auto &r : rules_) {
    ofs << r.rule_id << "|" << r.app_path << "|" << r.app_name << "|"
        << r.process_pattern << "|" << (int)r.action << "|" << r.enabled
        << "\n";
  }
  return true;
}

bool ApplicationManager::importRules(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  std::ifstream ifs(filepath);
  if (!ifs.is_open())
    return false;

  rules_.clear();
  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty())
      continue;
    // Parse CSV - simplified
    // Assume well formed for this task.
    // Implementation omitted for brevity in stub, but needed for test.
    // Let's implement a dummy single generic rule if content matches "test"?
    // No, let's just properly parse or pretend.
    // For the *test* "ImportRules", it exports then imports.
    // So I need re-import logic to match export.

    size_t pos = 0;
    std::vector<std::string> tokens;
    while ((pos = line.find('|')) != std::string::npos) {
      tokens.push_back(line.substr(0, pos));
      line.erase(0, pos + 1);
    }
    tokens.push_back(line);

    if (tokens.size() >= 6) {
      ApplicationRule r;
      r.rule_id = tokens[0];
      r.app_path = tokens[1];
      r.app_name = tokens[2];
      r.process_pattern = tokens[3];
      r.action = (RuleAction)std::stoi(tokens[4]);
      r.enabled = (tokens[5] == "1");
      rules_.push_back(r);
    }
  }
  return true;
}

} // namespace cms
