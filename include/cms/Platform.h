#ifndef CMS_PLATFORM_H
#define CMS_PLATFORM_H

#include "Common.h"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cms {
namespace platform {

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// CPU Information
struct CpuInfo {
  std::string brand;    // CPU brand (e.g., "Intel Core i7-9700K")
  int cores;            // Physical cores
  int logical_cores;    // Logical cores (with hyperthreading)
  double frequency_ghz; // Current frequency in GHz
};

// RAM Information
struct RamInfo {
  uint64_t total_bytes;     // Total RAM in bytes
  uint64_t available_bytes; // Available RAM in bytes
  uint64_t used_bytes;      // Used RAM in bytes
  double usage_percent;     // Usage percentage (0-100)
};

// Screen Resolution
struct Resolution {
  int width;          // Screen width in pixels
  int height;         // Screen height in pixels
  int bits_per_pixel; // Color depth
};

// Image Format
enum class ImageFormat {
  RGBA, // 32-bit RGBA
  RGB,  // 24-bit RGB
  JPEG, // JPEG compressed
  PNG   // PNG compressed
};

// Screen Image
struct ScreenImage {
  std::vector<uint8_t> data; // Image buffer
  int width;                 // Image width
  int height;                // Image height
  ImageFormat format;        // Pixel format
};

// Power Source
enum class PowerSource {
  AC,      // AC power (plugged in)
  Battery, // Running on battery
  Unknown  // Unknown power source
};

// Power Status
struct PowerStatus {
  PowerSource source;    // Current power source
  int battery_percent;   // Battery level 0-100, -1 if unknown
  bool is_charging;      // Is battery charging?
  int estimated_minutes; // Estimated battery time remaining, -1 if unknown
};

// Filter Mode
enum class FilterMode {
  AllowList, // Allow only listed domains
  BlockList  // Block only listed domains
};

// Filter Rule
struct FilterRule {
  std::string domain; // Domain to filter
  bool blocked;       // Is this domain blocked?
  std::string reason; // Reason for blocking
};

// ============================================================================
// PURE INTERFACES
// ============================================================================

// Platform Manager Interface
// Provides system information and platform detection
class IPlatformManager {
public:
  virtual ~IPlatformManager() = default;

  // Get platform name ("Windows", "Linux", "macOS")
  virtual std::string getPlatformName() = 0;

  // Get CPU information
  virtual CpuInfo getCPUInfo() = 0;

  // Get RAM information
  virtual RamInfo getRAMInfo() = 0;

  // Get screen resolution
  virtual Resolution getScreenResolution() = 0;

  // Get screen DPI scale factor (1.0, 1.25, 1.5, 2.0, etc.)
  virtual float getScreenScaleFactor() = 0;

  // Get list of running process IDs
  virtual std::vector<uint32_t> getRunningPids() = 0;

  // Get system hostname
  virtual std::string getHostname() = 0;

  // Get current logged-in username
  virtual std::string getUsername() = 0;
};

// Screen Capture Interface
// Provides screen and window capture functionality
class IScreenCapture {
public:
  virtual ~IScreenCapture() = default;

  // Capture entire screen
  virtual ScreenImage captureScreen() = 0;

  // Capture screen thumbnail (downscaled for bandwidth efficiency)
  // maxWidth/maxHeight: maximum dimensions, maintains aspect ratio
  virtual ScreenImage captureThumbnail(int maxWidth = 400,
                                       int maxHeight = 225) = 0;

  // Capture specific window (platform-specific handle)
  virtual ScreenImage captureWindow(void *window_handle) = 0;

  // Get screen dimensions (width, height)
  virtual std::pair<int, int> getScreenDimensions() = 0;

  // Get supported image formats
  virtual std::vector<ImageFormat> supportedFormats() = 0;
};

// Input Control Interface
// Provides keyboard and mouse locking functionality
class IInputControl {
public:
  virtual ~IInputControl() = default;

  // Lock keyboard (requires admin/root)
  virtual bool lockKeyboard() = 0;

  // Unlock keyboard
  virtual bool unlockKeyboard() = 0;

  // Lock mouse (requires admin/root)
  virtual bool lockMouse() = 0;

  // Unlock mouse
  virtual bool unlockMouse() = 0;

  // Check if input is currently locked
  virtual bool isInputLocked() = 0;

  // Input Simulation
  virtual void simulateMouseMove(int x, int y) = 0;
  virtual void
  simulateMouseClick(int x, int y, bool left,
                     bool down) = 0; // left=true (Left), left=false (Right).
                                     // down=true (Press), down=false (Release)
  virtual void simulateKeyPress(
      int key_code,
      bool down) = 0; // key_code is OS specific (Virtual Key on Windows)
};

// Power Control Interface
// Provides power management functionality
class IPowerControl {
public:
  virtual ~IPowerControl() = default;

  // Power off the system (requires admin/root)
  virtual bool powerOff() = 0;

  // Reboot the system (requires admin/root)
  virtual bool reboot() = 0;

  // Hibernate the system (requires admin/root)
  virtual bool hibernate() = 0;

  // Get current power status
  virtual PowerStatus getPowerStatus() = 0;
};

// Network Filter Interface
// Provides domain blocking/allowing functionality
class INetworkFilter {
public:
  virtual ~INetworkFilter() = default;

  // Block domains (requires admin/root)
  virtual bool blockDomains(const std::vector<std::string> &domains) = 0;

  // Allow domains (requires admin/root)
  virtual bool allowDomains(const std::vector<std::string> &domains) = 0;

  // Set filter mode to allow list
  virtual bool setAllowListMode() = 0;

  // Set filter mode to block list
  virtual bool setBlockListMode() = 0;

  // Get current filter rules
  virtual std::vector<FilterRule> getCurrentRules() = 0;

  // Set hosts file path (for testing)
  virtual void setHostsFilePath(const std::string &path) {}

  // Clear all rules (requires admin/root)
  virtual bool clearRules() = 0;
};

// ============================================================================
// PLATFORM CLASS
// ============================================================================

// Platform class implementing all interfaces
class Platform : public IPlatformManager,
                 public IScreenCapture,
                 public IInputControl,
                 public IPowerControl,
                 public INetworkFilter {
public:
  virtual ~Platform() = default;
};

// ============================================================================
// FACTORY FUNCTION
// ============================================================================

// Get platform instance (factory method)
// Returns platform-specific implementation based on compile-time detection
std::unique_ptr<Platform> getPlatformInstance();

} // namespace platform
} // namespace cms

#endif // CMS_PLATFORM_H
