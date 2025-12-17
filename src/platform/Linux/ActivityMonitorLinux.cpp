#include "cms/ActivityMonitor.h"
#include <stdexcept>

namespace cms {

class ActivityMonitorLinux : public ActivityMonitor {
public:
  bool startMonitoring() override { return false; }
  bool stopMonitoring() override { return false; }
  bool isMonitoring() const override { return false; }

  std::vector<ProcessInfo> getActiveApplications() override { return {}; }
  uint64_t getScreenIdleTime() override { return 0; }

  KeyboardActivityStats getKeyboardActivity() override { return {}; }
  MouseActivityStats getMouseActivity() override { return {}; }
  NetworkUsageStats getNetworkUsage() override { return {}; }

  float getCPUUsage() override { return 0.0f; }
  float getRAMUsage() override { return 0.0f; }

  ActivityReport getAllActivity() override { return {}; }
};

std::unique_ptr<ActivityMonitor> ActivityMonitor::Create() {
  return std::make_unique<ActivityMonitorLinux>();
}

} // namespace cms
