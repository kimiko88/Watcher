#include "cms/Platform.h"
#include "cms/Logger.h"

#ifdef CMS_PLATFORM_LINUX

#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

namespace cms {
namespace platform {

class LinuxPlatform : public Platform {
private:
    bool keyboard_locked_ = false;
    bool mouse_locked_ = false;

public:
    // IPlatformManager implementation
    std::string getPlatformName() override {
        return "Linux";
    }

    CpuInfo getCPUInfo() override {
        CpuInfo info;
        
        // Parse /proc/cpuinfo
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        
        info.cores = sysconf(_SC_NPROCESSORS_ONLN);
        info.logical_cores = info.cores;
        info.frequency_ghz = 2.5; // Would need to parse /proc/cpuinfo
        info.brand = "Linux CPU";
        
        return info;
    }

    RamInfo getRAMInfo() override {
        RamInfo info;
        
        struct sysinfo si;
        sysinfo(&si);
        
        info.total_bytes = si.totalram * si.mem_unit;
        info.available_bytes = si.freeram * si.mem_unit;
        info.used_bytes = info.total_bytes - info.available_bytes;
        info.usage_percent = (static_cast<double>(info.used_bytes) / info.total_bytes) * 100.0;
        
        return info;
    }

    Resolution getScreenResolution() override {
        Resolution res;
        // TODO: Use X11 or Wayland to get actual resolution
        res.width = 1920;
        res.height = 1080;
        res.bits_per_pixel = 32;
        LOG_WARNING("getScreenResolution stub - returning default resolution");
        return res;
    }

    float getScreenScaleFactor() override {
        // TODO: Parse Xresources or query Wayland
        LOG_WARNING("getScreenScaleFactor stub - returning 1.0");
        return 1.0f;
    }

    // IScreenCapture implementation  
    ScreenImage captureScreen() override {
        LOG_WARNING("captureScreen not implemented on Linux");
        // TODO: Implement using X11 XGetImage or Wayland screencopy
        return ScreenImage{};
    }

    ScreenImage captureWindow(void* window_handle) override {
        LOG_WARNING("captureWindow not implemented on Linux");
        return ScreenImage{};
    }

    std::pair<int, int> getScreenDimensions() override {
        auto res = getScreenResolution();
        return {res.width, res.height};
    }

    std::vector<ImageFormat> supportedFormats() override {
        return {ImageFormat::RGBA, ImageFormat::RGB};
    }

    // IInputControl implementation
    bool lockKeyboard() override {
        LOG_WARNING("lockKeyboard not implemented on Linux");
        // TODO: Use evdev to grab /dev/input devices
        return false;
    }

    bool unlockKeyboard() override {
        LOG_WARNING("unlockKeyboard not implemented on Linux");
        return false;
    }

    bool lockMouse() override {
        LOG_WARNING("lockMouse not implemented on Linux");
        return false;
    }

    bool unlockMouse() override {
        LOG_WARNING("unlockMouse not implemented on Linux");
        return false;
    }

    bool isInputLocked() override {
        return keyboard_locked_ || mouse_locked_;
    }

    // IPowerControl implementation
    bool powerOff() override {
        LOG_WARNING("powerOff not implemented on Linux");
        // TODO: Use systemd D-Bus API
        return false;
    }

    bool reboot() override {
        LOG_WARNING("reboot not implemented on Linux");
        return false;
    }

    bool hibernate() override {
        LOG_WARNING("hibernate not implemented on Linux");
        return false;
    }

    PowerStatus getPowerStatus() override {
        PowerStatus status;
        // TODO: Parse /sys/class/power_supply/
        status.source = PowerSource::Unknown;
        status.battery_percent = -1;
        status.is_charging = false;
        status.estimated_minutes = -1;
        LOG_WARNING("getPowerStatus stub - returning unknown status");
        return status;
    }

    // INetworkFilter implementation
    bool blockDomains(const std::vector<std::string>& domains) override {
        LOG_WARNING("blockDomains not implemented on Linux");
        // TODO: Use iptables or modify /etc/hosts
        return false;
    }

    bool allowDomains(const std::vector<std::string>& domains) override {
        LOG_WARNING("allowDomains not implemented on Linux");
        return false;
    }

    bool setAllowListMode() override {
        LOG_WARNING("setAllowListMode not implemented on Linux");
        return false;
    }

    bool setBlockListMode() override {
        LOG_WARNING("setBlockListMode not implemented on Linux");
        return false;
    }

    std::vector<FilterRule> getCurrentRules() override {
        return {};
    }
};

// Factory implementation
std::unique_ptr<Platform> getPlatformInstance() {
    return std::make_unique<LinuxPlatform>();
}

} // namespace platform
} // namespace cms

#endif // CMS_PLATFORM_LINUX
