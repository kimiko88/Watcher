#include "cms/Platform.h"
#include "cms/Logger.h"

#ifdef CMS_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <powrprof.h>
#include <winuser.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstdlib>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "PowrProf.lib")

namespace cms {
namespace platform {

class WindowsPlatform : public Platform {
private:
    bool keyboard_locked_ = false;
    bool mouse_locked_ = false;

public:
    // IPlatformManager implementation
    std::string getPlatformName() override {
        return "Windows";
    }

    CpuInfo getCPUInfo() override {
        CpuInfo info;
        
        // Get CPU info from registry or WMI (simplified version)
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        info.cores = sysInfo.dwNumberOfProcessors;
        info.logical_cores = sysInfo.dwNumberOfProcessors;
        
        // Frequency (simplified - would need to read from registry for accurate value)
        info.frequency_ghz = 2.5; // Default estimate
        
        // Brand (would need registry query for actual brand)
        info.brand = "Windows CPU";
        
        return info;
    }

    RamInfo getRAMInfo() override {
        RamInfo info;
        
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        
        info.total_bytes = memInfo.ullTotalPhys;
        info.available_bytes = memInfo.ullAvailPhys;
        info.used_bytes = info.total_bytes - info.available_bytes;
        info.usage_percent = static_cast<double>(memInfo.dwMemoryLoad);
        
        return info;
    }

    Resolution getScreenResolution() override {
        Resolution res;
        
        res.width = GetSystemMetrics(SM_CXSCREEN);
        res.height = GetSystemMetrics(SM_CYSCREEN);
        res.bits_per_pixel = 32; // Modern systems are typically 32-bit
        
        return res;
    }

    float getScreenScaleFactor() override {
        // Get DPI scaling
        HDC screen = GetDC(NULL);
        int dpi = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(NULL, screen);
        
        // 96 DPI is 100% scaling (1.0)
        return static_cast<float>(dpi) / 96.0f;
    }

    // IScreenCapture implementation
    ScreenImage captureScreen() override {
        ScreenImage image;
        
        try {
            // Get screen DC
            HDC screenDC = GetDC(NULL);
            HDC memDC = CreateCompatibleDC(screenDC);
            
            int width = GetSystemMetrics(SM_CXSCREEN);
            int height = GetSystemMetrics(SM_CYSCREEN);
            
            // Create bitmap
            HBITMAP bitmap = CreateCompatibleBitmap(screenDC, width, height);
            HBITMAP old = (HBITMAP)SelectObject(memDC, bitmap);
            
            // Copy screen to bitmap
            BitBlt(memDC, 0, 0, width, height, screenDC, 0, 0, SRCCOPY);
            
            // Get bitmap bits
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height; // Top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            
            size_t dataSize = width * height * 4;
            image.data.resize(dataSize);
            
            GetDIBits(memDC, bitmap, 0, height, image.data.data(), &bmi, DIB_RGB_COLORS);
            
            image.width = width;
            image.height = height;
            image.format = ImageFormat::RGBA;
            
            // Cleanup
            SelectObject(memDC, old);
            DeleteObject(bitmap);
            DeleteDC(memDC);
            ReleaseDC(NULL, screenDC);
            
        } catch (...) {
            LOG_ERROR("Failed to capture screen");
        }
        
        return image;
    }

    ScreenImage captureWindow(void* window_handle) override {
        ScreenImage image;
        
        if (!window_handle) {
            return image;
        }
        
        HWND hwnd = static_cast<HWND>(window_handle);
        
        try {
            // Get window rect
            RECT rect;
            GetWindowRect(hwnd, &rect);
            
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            
            // Get window DC
            HDC windowDC = GetDC(hwnd);
            HDC memDC = CreateCompatibleDC(windowDC);
            
            // Create bitmap
            HBITMAP bitmap = CreateCompatibleBitmap(windowDC, width, height);
            HBITMAP old = (HBITMAP)SelectObject(memDC, bitmap);
            
            // Copy window to bitmap
            BitBlt(memDC, 0, 0, width, height, windowDC, 0, 0, SRCCOPY);
            
            // Get bitmap bits
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            
            size_t dataSize = width * height * 4;
            image.data.resize(dataSize);
            
            GetDIBits(memDC, bitmap, 0, height, image.data.data(), &bmi, DIB_RGB_COLORS);
            
            image.width = width;
            image.height = height;
            image.format = ImageFormat::RGBA;
            
            // Cleanup
            SelectObject(memDC, old);
            DeleteObject(bitmap);
            DeleteDC(memDC);
            ReleaseDC(hwnd, windowDC);
            
        } catch (...) {
            LOG_ERROR("Failed to capture window");
        }
        
        return image;
    }

    std::pair<int, int> getScreenDimensions() override {
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);
        return {width, height};
    }

    std::vector<ImageFormat> supportedFormats() override {
        return {ImageFormat::RGBA, ImageFormat::RGB};
    }

    // IInputControl implementation
    bool lockKeyboard() override {
        // BlockInput requires admin privileges
        BOOL result = BlockInput(TRUE);
        if (result) {
            keyboard_locked_ = true;
            LOG_INFO("Keyboard locked");
        } else {
            LOG_WARNING("Failed to lock keyboard (requires admin privileges)");
        }
        return result != 0;
    }

    bool unlockKeyboard() override {
        BOOL result = BlockInput(FALSE);
        if (result) {
            keyboard_locked_ = false;
            LOG_INFO("Keyboard unlocked");
        }
        return result != 0;
    }

    bool lockMouse() override {
        // On Windows, BlockInput locks both keyboard and mouse
        BOOL result = BlockInput(TRUE);
        if (result) {
            mouse_locked_ = true;
            LOG_INFO("Mouse locked");
        } else {
            LOG_WARNING("Failed to lock mouse (requires admin privileges)");
        }
        return result != 0;
    }

    bool unlockMouse() override {
        BOOL result = BlockInput(FALSE);
        if (result) {
            mouse_locked_ = false;
            LOG_INFO("Mouse unlocked");
        }
        return result != 0;
    }

    bool isInputLocked() override {
        return keyboard_locked_ || mouse_locked_;
    }

    // IPowerControl implementation
    bool powerOff() override {
        LOG_WARNING("Power off requested");
        // ExitWindowsEx requires appropriate privileges
        BOOL result = ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
        return result != 0;
    }

    bool reboot() override {
        LOG_WARNING("Reboot requested");
        BOOL result = ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
        return result != 0;
    }

    bool hibernate() override {
        LOG_WARNING("Hibernate requested");
        SetSuspendState(TRUE, FALSE, FALSE);
        return true;
    }

    PowerStatus getPowerStatus() override {
        PowerStatus status;
        
        SYSTEM_POWER_STATUS powerStatus;
        if (GetSystemPowerStatus(&powerStatus)) {
            // Determine power source
            if (powerStatus.ACLineStatus == 1) {
                status.source = PowerSource::AC;
            } else if (powerStatus.ACLineStatus == 0) {
                status.source = PowerSource::Battery;
            } else {
                status.source = PowerSource::Unknown;
            }
            
            // Battery percentage
            if (powerStatus.BatteryLifePercent != 255) {
                status.battery_percent = powerStatus.BatteryLifePercent;
            } else {
                status.battery_percent = -1;
            }
            
            // Charging status
            status.is_charging = (powerStatus.BatteryFlag & 8) != 0;
            
            // Estimated time
            if (powerStatus.BatteryLifeTime != (DWORD)-1) {
                status.estimated_minutes = powerStatus.BatteryLifeTime / 60;
            } else {
                status.estimated_minutes = -1;
            }
        } else {
            status.source = PowerSource::Unknown;
            status.battery_percent = -1;
            status.is_charging = false;
            status.estimated_minutes = -1;
        }
        
        return status;
    }

    // INetworkFilter implementation
    bool blockDomains(const std::vector<std::string>& domains) override {
        // Use Windows hosts file for domain blocking
        // Path: C:\Windows\System32\drivers\etc\hosts
        // Requires administrator privileges
        
        std::string hostsPath = std::string(std::getenv("SYSTEMROOT")) + "\\System32\\drivers\\etc\\hosts";
        
        try {
            // Read existing hosts file
            std::ifstream hostsIn(hostsPath);
            if (!hostsIn.is_open()) {
                LOG_ERROR("Failed to open hosts file for reading (admin required)");
                return false;
            }
            
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(hostsIn, line)) {
                lines.push_back(line);
            }
            hostsIn.close();
            
            // Append blocked domains
            std::ofstream hostsOut(hostsPath, std::ios::app);
            if (!hostsOut.is_open()) {
                LOG_ERROR("Failed to open hosts file for writing (admin required)");
                return false;
            }
            
            // Add CMS marker if not present
            bool hasMarker = false;
            for (const auto& l : lines) {
                if (l.find("# CMS Blocked Domains") != std::string::npos) {
                    hasMarker = true;
                    break;
                }
            }
            
            if (!hasMarker) {
                hostsOut << "\n# CMS Blocked Domains\n";
            }
            
            // Block each domain by redirecting to 127.0.0.1
            for (const auto& domain : domains) {
                // Check if already blocked
                bool alreadyBlocked = false;
                for (const auto& l : lines) {
                    if (l.find(domain) != std::string::npos) {
                        alreadyBlocked = true;
                        break;
                    }
                }
                
                if (!alreadyBlocked) {
                    hostsOut << "127.0.0.1 " << domain << "\n";
                    LOG_INFO("Blocked domain: " + domain);
                }
            }
            
            hostsOut.close();
            
            // Flush DNS cache
            std::system("ipconfig /flushdns >nul 2>&1");
            
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Error blocking domains: ") + e.what());
            return false;
        }
    }

    bool allowDomains(const std::vector<std::string>& domains) override {
        // Remove domains from hosts file
        std::string hostsPath = std::string(std::getenv("SYSTEMROOT")) + "\\System32\\drivers\\etc\\hosts";
        
        try {
            // Read existing hosts file
            std::ifstream hostsIn(hostsPath);
            if (!hostsIn.is_open()) {
                LOG_ERROR("Failed to open hosts file for reading (admin required)");
                return false;
            }
            
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(hostsIn, line)) {
                bool shouldKeep = true;
                
                // Check if line contains any of the domains to allow
                for (const auto& domain : domains) {
                    if (line.find(domain) != std::string::npos && 
                        line.find("127.0.0.1") != std::string::npos) {
                        shouldKeep = false;
                        LOG_INFO("Allowed domain: " + domain);
                        break;
                    }
                }
                
                if (shouldKeep) {
                    lines.push_back(line);
                }
            }
            hostsIn.close();
            
            // Write back filtered content
            std::ofstream hostsOut(hostsPath, std::ios::trunc);
            if (!hostsOut.is_open()) {
                LOG_ERROR("Failed to open hosts file for writing (admin required)");
                return false;
            }
            
            for (const auto& l : lines) {
                hostsOut << l << "\n";
            }
            hostsOut.close();
            
            // Flush DNS cache
            std::system("ipconfig /flushdns >nul 2>&1");
            
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Error allowing domains: ") + e.what());
            return false;
        }
    }

    bool setAllowListMode() override {
        // Allow list mode: block all except allowed
        // This is complex with hosts file - would need firewall rules
        LOG_WARNING("setAllowListMode not implemented (use blockDomains for block list)");
        return false;
    }

    bool setBlockListMode() override {
        // Block list mode is the default with hosts file
        LOG_INFO("Block list mode active (hosts file based)");
        return true;
    }

    std::vector<FilterRule> getCurrentRules() override {
        std::vector<FilterRule> rules;
        std::string hostsPath = std::string(std::getenv("SYSTEMROOT")) + "\\System32\\drivers\\etc\\hosts";
        
        try {
            std::ifstream hostsIn(hostsPath);
            if (!hostsIn.is_open()) {
                return rules;
            }
            
            std::string line;
            bool inCMSSection = false;
            
            while (std::getline(hostsIn, line)) {
                if (line.find("# CMS Blocked Domains") != std::string::npos) {
                    inCMSSection = true;
                    continue;
                }
                
                if (inCMSSection && line.find("127.0.0.1") != std::string::npos) {
                    // Extract domain
                    std::istringstream iss(line);
                    std::string ip, domain;
                    iss >> ip >> domain;
                    
                    if (!domain.empty()) {
                        FilterRule rule;
                        rule.domain = domain;
                        rule.blocked = true;
                        rule.reason = "CMS blocked via hosts file";
                        rules.push_back(rule);
                    }
                }
            }
            
            hostsIn.close();
            
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Error reading filter rules: ") + e.what());
        }
        
        return rules;
    }
};

// Factory implementation
std::unique_ptr<Platform> getPlatformInstance() {
    return std::make_unique<WindowsPlatform>();
}

} // namespace platform
} // namespace cms

#endif // CMS_PLATFORM_WINDOWS
