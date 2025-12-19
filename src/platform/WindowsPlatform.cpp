#define _CRT_SECURE_NO_WARNINGS

// Standard C++ headers
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// Windows headers
#ifdef _WIN32
#define NOMINMAX
// Hack: Define _AMD64_ and _M_AMD64 if missing to fix "No Target Architecture"
// and WINAPI definition
#if !defined(_AMD64_) && !defined(_X86_) && !defined(_ARM_) && !defined(_ARM64_)
#define _AMD64_
#ifndef _M_AMD64
#define _M_AMD64 100
#endif
#endif
#include <minwindef.h>
#include <psapi.h>
#include <windows.h>
#include <winuser.h>

#endif

// Project headers
#include "cms/Logger.h"
#include "cms/Platform.h"

// Ensure CMS_PLATFORM_WINDOWS is defined if we are on Windows
#if defined(_WIN32) && !defined(CMS_PLATFORM_WINDOWS)
#define CMS_PLATFORM_WINDOWS
#endif

#ifdef CMS_PLATFORM_WINDOWS

// Manually declare SetSuspendState if needed (usually in powrprof.h)
extern "C" {
BOOLEAN WINAPI SetSuspendState(BOOLEAN bHibernate, BOOLEAN bForce,
                               BOOLEAN bWakeupEventsDisabled);
}

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "psapi.lib")
namespace cms {
namespace platform {

static std::string g_hosts_file_path = []() {
  const char *systemRoot = std::getenv("SYSTEMROOT");
  if (systemRoot) {
    return std::string(systemRoot) + "\\System32\\drivers\\etc\\hosts";
  }
  return std::string("C:\\Windows\\System32\\drivers\\etc\\hosts");
}();

class WindowsPlatform : public Platform {
private:
  bool keyboard_locked_ = false;
  bool mouse_locked_ = false;

public:
  std::string getPlatformName() override { return "Windows"; }

  CpuInfo getCPUInfo() override {
    CpuInfo info;

    // CPU Info
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    info.cores = sysInfo.dwNumberOfProcessors;
    info.logical_cores = sysInfo.dwNumberOfProcessors;
    info.frequency_ghz = 2.5;   // Placeholder
    info.brand = "Windows CPU"; // Placeholder

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
    res.bits_per_pixel = 32;
    return res;
  }

  float getScreenScaleFactor() override {
    HDC screen = GetDC(NULL);
    int dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(NULL, screen);
    return static_cast<float>(dpi) / 96.0f;
  }

  std::vector<uint32_t> getRunningPids() override {
    std::vector<uint32_t> pids;
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
      cProcesses = cbNeeded / sizeof(DWORD);
      for (unsigned int i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
          pids.push_back(aProcesses[i]);
        }
      }
    }
    return pids;
  }

  ScreenImage captureScreen() override {
    ScreenImage image;
    try {
      HDC screenDC = GetDC(NULL);
      HDC memDC = CreateCompatibleDC(screenDC);
      int width = GetSystemMetrics(SM_CXSCREEN);
      int height = GetSystemMetrics(SM_CYSCREEN);
      HBITMAP bitmap = CreateCompatibleBitmap(screenDC, width, height);
      HBITMAP old = (HBITMAP)SelectObject(memDC, bitmap);
      BitBlt(memDC, 0, 0, width, height, screenDC, 0, 0, SRCCOPY);

      BITMAPINFO bmi = {};
      bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth = width;
      bmi.bmiHeader.biHeight = -height;
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;

      size_t dataSize = width * height * 4;
      image.data.resize(dataSize);
      GetDIBits(memDC, bitmap, 0, height, image.data.data(), &bmi,
                DIB_RGB_COLORS);

      image.width = width;
      image.height = height;
      image.format = ImageFormat::RGBA;

      SelectObject(memDC, old);
      DeleteObject(bitmap);
      DeleteDC(memDC);
      ReleaseDC(NULL, screenDC);
    } catch (...) {
      LOG_ERROR("Failed to capture screen");
    }
    return image;
  }

  ScreenImage captureWindow(void *window_handle) override {
    ScreenImage image;
    if (!window_handle)
      return image;
    HWND hwnd = static_cast<HWND>(window_handle);
    try {
      RECT rect;
      GetWindowRect(hwnd, &rect);
      int width = rect.right - rect.left;
      int height = rect.bottom - rect.top;
      HDC windowDC = GetDC(hwnd);
      HDC memDC = CreateCompatibleDC(windowDC);
      HBITMAP bitmap = CreateCompatibleBitmap(windowDC, width, height);
      HBITMAP old = (HBITMAP)SelectObject(memDC, bitmap);
      BitBlt(memDC, 0, 0, width, height, windowDC, 0, 0, SRCCOPY);

      BITMAPINFO bmi = {};
      bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth = width;
      bmi.bmiHeader.biHeight = -height;
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;

      size_t dataSize = width * height * 4;
      image.data.resize(dataSize);
      GetDIBits(memDC, bitmap, 0, height, image.data.data(), &bmi,
                DIB_RGB_COLORS);

      image.width = width;
      image.height = height;
      image.format = ImageFormat::RGBA;

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
    return {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
  }

  std::vector<ImageFormat> supportedFormats() override {
    return {ImageFormat::RGBA, ImageFormat::RGB};
  }

  bool lockKeyboard() override {
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

  bool isInputLocked() override { return keyboard_locked_ || mouse_locked_; }

  bool powerOff() override {
    LOG_WARNING("Power off requested");
    return ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER) !=
           0;
  }

  bool reboot() override {
    LOG_WARNING("Reboot requested");
    return ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER) != 0;
  }

  bool hibernate() override {
    LOG_WARNING("Hibernate requested");
    return SetSuspendState(TRUE, FALSE, FALSE) != 0;
  }

  PowerStatus getPowerStatus() override {
    PowerStatus status;
    SYSTEM_POWER_STATUS powerStatus;
    if (GetSystemPowerStatus(&powerStatus)) {
      status.source = (powerStatus.ACLineStatus == 1)   ? PowerSource::AC
                      : (powerStatus.ACLineStatus == 0) ? PowerSource::Battery
                                                        : PowerSource::Unknown;
      status.battery_percent = (powerStatus.BatteryLifePercent != 255)
                                   ? powerStatus.BatteryLifePercent
                                   : -1;
      status.is_charging = (powerStatus.BatteryFlag & 8) != 0;
      status.estimated_minutes = (powerStatus.BatteryLifeTime != (DWORD)-1)
                                     ? powerStatus.BatteryLifeTime / 60
                                     : -1;
    } else {
      status.source = PowerSource::Unknown;
      status.battery_percent = -1;
      status.is_charging = false;
      status.estimated_minutes = -1;
    }
    return status;
  }

  bool blockDomains(const std::vector<std::string> &domains) override {
    std::string hostsPath = g_hosts_file_path;
    try {
      std::ifstream hostsIn(hostsPath);
      if (!hostsIn.is_open()) {
        LOG_ERROR("Failed to open hosts file for reading (admin required)");
        return false;
      }
      std::vector<std::string> lines;
      std::string line;
      while (std::getline(hostsIn, line))
        lines.push_back(line);
      hostsIn.close();

      std::ofstream hostsOut(hostsPath, std::ios::app);
      if (!hostsOut.is_open()) {
        LOG_ERROR("Failed to open hosts file for writing (admin required)");
        return false;
      }

      bool hasMarker = false;
      for (const auto &l : lines) {
        if (l.find("# CMS Blocked Domains") != std::string::npos) {
          hasMarker = true;
          break;
        }
      }
      if (!hasMarker)
        hostsOut << "\n# CMS Blocked Domains\n";

      for (const auto &domain : domains) {
        bool alreadyBlocked = false;
        for (const auto &l : lines) {
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
      std::system("ipconfig /flushdns >nul 2>&1");
      return true;
    } catch (const std::exception &e) {
      LOG_ERROR(std::string("Error blocking domains: ") + e.what());
      return false;
    }
  }

  bool allowDomains(const std::vector<std::string> &domains) override {
    std::string hostsPath = g_hosts_file_path;
    try {
      std::ifstream hostsIn(hostsPath);
      if (!hostsIn.is_open()) {
        LOG_ERROR("Failed to open hosts file for reading (admin required)");
        return false;
      }
      std::vector<std::string> lines;
      std::string line;
      while (std::getline(hostsIn, line)) {
        bool shouldKeep = true;
        for (const auto &domain : domains) {
          if (line.find(domain) != std::string::npos &&
              line.find("127.0.0.1") != std::string::npos) {
            shouldKeep = false;
            LOG_INFO("Allowed domain: " + domain);
            break;
          }
        }
        if (shouldKeep)
          lines.push_back(line);
      }
      hostsIn.close();

      std::ofstream hostsOut(hostsPath, std::ios::trunc);
      if (!hostsOut.is_open()) {
        LOG_ERROR("Failed to open hosts file for writing (admin required)");
        return false;
      }
      for (const auto &l : lines)
        hostsOut << l << "\n";
      hostsOut.close();
      std::system("ipconfig /flushdns >nul 2>&1");
      return true;
    } catch (const std::exception &e) {
      LOG_ERROR(std::string("Error allowing domains: ") + e.what());
      return false;
    }
  }

  bool setAllowListMode() override {
    LOG_WARNING("setAllowListMode not implemented (use blockDomains for block "
                "list)");
    return false;
  }

  bool setBlockListMode() override {
    LOG_INFO("Block list mode active (hosts file based)");
    return true;
  }

  std::vector<FilterRule> getCurrentRules() override {
    std::vector<FilterRule> rules;
    std::string hostsPath = g_hosts_file_path;
    try {
      std::ifstream hostsIn(hostsPath);
      if (!hostsIn.is_open())
        return rules;
      std::string line;
      bool inCMSSection = false;
      while (std::getline(hostsIn, line)) {
        if (line.find("# CMS Blocked Domains") != std::string::npos) {
          inCMSSection = true;
          continue;
        }
        if (inCMSSection && line.find("127.0.0.1") != std::string::npos) {
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
    } catch (const std::exception &e) {
      LOG_ERROR(std::string("Error reading filter rules: ") + e.what());
    }
    return rules;
  }

  void setHostsFilePath(const std::string &path) override {
    g_hosts_file_path = path;
  }
};

std::unique_ptr<Platform> getPlatformInstance() {
  return std::make_unique<WindowsPlatform>();
}

} // namespace platform
} // namespace cms

#endif // CMS_PLATFORM_WINDOWS
