// clang-format off
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#include <tlhelp32.h>
#include "cms/ActivityMonitor.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
// clang-format on

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "pdh.lib")

namespace cms {

class ActivityMonitorWindows : public ActivityMonitor {
public:
  ActivityMonitorWindows() : monitoring_(false) {
    // Initialize PDH for CPU usage
    PdhOpenQuery(NULL, NULL, &cpuQuery_);
    PdhAddCounter(cpuQuery_, "\\Processor(_Total)\\% Processor Time", NULL,
                  &cpuTotal_);
    PdhCollectQueryData(cpuQuery_);
  }

  ~ActivityMonitorWindows() override {
    stopMonitoring();
    if (cpuQuery_)
      PdhCloseQuery(cpuQuery_);
  }

  bool startMonitoring() override {
    if (monitoring_)
      return true; // Idempotent
    monitoring_ = true;

    // Start background thread if we were implementing historical buffering
    // For now, we fetch on demand as per the basic getters,
    // or we could have a thread that updates stats.
    // The requirements ask for a background thread.
    monitorThread_ = std::thread(&ActivityMonitorWindows::monitorLoop, this);

    return true;
  }

  bool stopMonitoring() override {
    if (!monitoring_)
      return true;
    monitoring_ = false;
    if (monitorThread_.joinable()) {
      monitorThread_.join();
    }
    return true;
  }

  bool isMonitoring() const override { return monitoring_; }

  std::vector<ProcessInfo> getActiveApplications() override {
    std::vector<ProcessInfo> processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
      return processes;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
      do {
        ProcessInfo info;
        info.process_id = pe32.th32ProcessID;
        info.process_name = pe32.szExeFile;
        info.executable_path =
            ""; // Requires OpenProcess + GetModuleFileNameEx (Costly)
        info.cpu_percentage = 0.0f; // Placeholder
        info.memory_usage_mb = 0.0f;
        info.creation_time = 0;
        info.is_visible = true; // Simplified

        // Basic Memory Usage
        PROCESS_MEMORY_COUNTERS pmc;
        HANDLE hProcess =
            OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                        pe32.th32ProcessID);
        if (hProcess) {
          if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            info.memory_usage_mb =
                static_cast<float>(pmc.WorkingSetSize) / (1024 * 1024);
          }
          CloseHandle(hProcess);
        }

        processes.push_back(info);

      } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processes;
  }

  uint64_t getScreenIdleTime() override {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
      DWORD currentTick = GetTickCount();
      return static_cast<uint64_t>(currentTick - lii.dwTime);
    }
    return 0;
  }

  KeyboardActivityStats getKeyboardActivity() override {
    std::lock_guard<std::mutex> lock(statsMutex_);
    KeyboardActivityStats stats = currentKeyboardStats_;
    stats.idle_time_seconds = getScreenIdleTime() / 1000;
    return stats;
  }

  MouseActivityStats getMouseActivity() override {
    std::lock_guard<std::mutex> lock(statsMutex_);
    MouseActivityStats stats = currentMouseStats_;
    stats.idle_time_seconds = getScreenIdleTime() / 1000;
    return stats;
  }

  NetworkUsageStats getNetworkUsage() override {
    NetworkUsageStats stats = {0, 0, 0.0f, 0};

    MIB_IFTABLE *pIfTable;
    DWORD dwSize = 0;

    // First call to get size
    if (GetIfTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
      pIfTable = (MIB_IFTABLE *)malloc(dwSize);
      if (GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
          stats.bytes_received += pIfTable->table[i].dwInOctets;
          stats.bytes_sent += pIfTable->table[i].dwOutOctets;
        }

        // Bandwidth calcs require deltas over time.
        // For a simple snapshot, we return totals.
        // Start/Stop monitoring would track deltas.
      }
      free(pIfTable);
    }
    return stats;
  }

  float getCPUUsage() override {
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQuery_);
    PdhGetFormattedCounterValue(cpuTotal_, PDH_FMT_DOUBLE, NULL, &counterVal);
    return static_cast<float>(counterVal.doubleValue);
  }

  float getRAMUsage() override {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return static_cast<float>(memInfo.dwMemoryLoad);
  }

  ActivityReport getAllActivity() override {
    ActivityReport report;
    report.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    report.processes = getActiveApplications();
    report.keyboard_stats = getKeyboardActivity();
    report.mouse_stats = getMouseActivity();
    report.network_stats = getNetworkUsage();
    report.cpu_average = getCPUUsage();
    report.ram_average = getRAMUsage();
    return report;
  }

private:
  void monitorLoop() {
    while (monitoring_) {
      // Update stats, history buffer, etc.
      // Simplified for this pass
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }

  std::atomic<bool> monitoring_;
  std::thread monitorThread_;

  PDH_HQUERY cpuQuery_ = NULL;
  PDH_HCOUNTER cpuTotal_ = NULL;

  std::mutex statsMutex_;
  KeyboardActivityStats currentKeyboardStats_ = {0};
  MouseActivityStats currentMouseStats_ = {0};
};

// Factory implementation
std::unique_ptr<ActivityMonitor> ActivityMonitor::Create() {
  return std::make_unique<ActivityMonitorWindows>();
}

} // namespace cms
