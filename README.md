# Watcher - Classroom Management System

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)]()
[![C++](https://img.shields.io/badge/C++-17-blue.svg)]()
[![Qt](https://img.shields.io/badge/Qt-6-green.svg)]()

**[🇮🇹 Versione Italiana](README_IT.md)**

A comprehensive classroom management system (CMS) for educational environments, enabling teachers to monitor and control student computers from a central master application.

## 🎯 Features

### 📸 Real-Time Monitoring
- **Live Screenshot Capture**: Capture full-resolution screenshots of student screens with DPI-aware rendering
- **Automatic Thumbnail Updates**: View real-time thumbnail previews of all connected clients
- **Multi-Monitor Support**: Correctly captures entire screen across multiple displays
- **High-Quality Export**: Save screenshots to `Pictures/Screenshots` in PNG format with timestamp

### 🔒 Screen Control
- **Input Locking**: Lock both keyboard and mouse input on student machines
- **Selective Unlock**: Independently control keyboard and mouse lock states
- **Admin-Level Control**: Requires appropriate privileges for maximum security

### 🌐 Network Filtering
- **Domain Blocking**: Block access to specific websites via hosts file modification
- **Domain Allow-List**: Whitelist-only mode for strict internet control
- **Dynamic Rule Management**: Add/remove domains without system restart
- **Automatic DNS Flush**: Changes take effect immediately

### 🚫 Application Control
- **Process Blocking**: Prevent specific applications from running
- **Rule Persistence**: Application rules saved across sessions
- **Real-Time Monitoring**: Continuously scans for and blocks forbidden applications
- **Flexible Filtering**: Support for both blacklist and whitelist modes

### ⚡ Power Management
- **Remote Shutdown**: Power off student machines remotely
- **Remote Reboot**: Restart client computers
- **Hibernate Support**: Put machines into low-power state
- **Battery Status**: Monitor battery level and charging state (laptops)

### 🖥️ Client Information
- **System Details**: View CPU, RAM, and screen resolution
- **Network Information**: Display IP address (IPv4/IPv6)
- **Hostname Display**: Show computer name and username
- **Connection Status**: Real-time monitoring of client connectivity

## 🏗️ Architecture

### Master Application
The teacher-facing GUI application built with Qt6:
- **Dashboard**: Grid view of all connected clients
- **Screenshot Viewer**: Full-screen image viewer with zoom capabilities
- **Policy Management**: Configure domain and application filtering
- **Broadcast Commands**: Send commands to all or selected clients

### Client Service
Background service running on student machines:
- **Lightweight**: Minimal resource usage
- **Auto-Reconnect**: Automatically reconnects to master if connection is lost
- **Secure Communication**: JSON-based protocol over TCP
- **Platform-Specific**: Optimized Windows API integration

### Communication Protocol
- **TCP-based**: Reliable message delivery
- **JSON Messages**: Human-readable command format
- **Newline-Delimited**: Efficient message parsing
- **Base64 Encoding**: Screenshot data transmission

## 📋 Requirements

### Master Application
- Windows 10/11
- **Qt 6.5 or higher** (required for GUI)
- CMake 3.15+
- MSVC 2019 or newer
- Visual Studio 2019/2022 with C++ Desktop Development workload

### Client Service
- Windows 10/11
- CMake 3.15+
- MSVC 2019 or newer
- Administrator privileges (for input locking and domain filtering)
- Network connectivity to master

## 🔧 Qt Installation

### Installing Qt 6

1. **Download Qt Online Installer**
   - Visit [Qt Official Website](https://www.qt.io/download-qt-installer)
   - Download the Qt Online Installer for Windows

2. **Install Qt 6.5+ with Required Components**
   ```
   During installation, select:
   ✓ Qt 6.5 (or newer)
   ✓ MSVC 2019 64-bit (or MSVC 2022 64-bit)
   ✓ Qt 5 Compatibility Module
   ✓ Additional Libraries (if prompted)
   ```

3. **Note the Installation Path**
   - Default: `C:\Qt\6.10.1\msvc2022_64`
   - You'll need this path for CMake configuration

### Setting Qt Path

Add Qt to your system PATH or use CMake prefix:

**Option 1: Set CMAKE_PREFIX_PATH**
```bash
set CMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2022_64
```

**Option 2: Use Qt CMake Path**
```bash
set Qt6_DIR=C:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6
```

## 🚀 Quick Start

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/Watcher.git
cd Watcher

# Create build directory
mkdir build
cd build

# Configure with CMake (specify Qt path)
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2022_64

# Alternative: specify Qt6_DIR
cmake .. -DQt6_DIR=C:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6

# Build master application (with Qt GUI)
cmake --build . --target cms_master --config Release

# Build client service (no Qt dependency)
cmake --build . --target cms_client --config Release
```

### Visual Studio Build

```bash
# Generate Visual Studio solution
cmake -B build -G "Visual Studio 16 2019" -A x64 ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2022_64

# Open in Visual Studio
start build\Watcher.sln

# Or build from command line
cmake --build build --config Release
```

### Configuration

#### Master Setup
1. Launch `cms_master.exe`
2. The server will automatically start on port `5555`
3. Clients will connect automatically

#### Client Setup
1. Edit `client_config.json`:
```json
{
  "master_address": "192.168.1.100",
  "master_port": 5555,
  "machine_id": "student-pc-01",
  "encryption_enabled": false,
  "log_level": "INFO"
}
```

2. Run `cms_client.exe` (requires administrator privileges)

## 🔧 Usage

### Taking Screenshots
1. Select a client from the grid (or use auto-selection if only one client)
2. Click the **📸 Screenshot** button in the toolbar
3. View the screenshot in the popup dialog
4. Use zoom controls or save to disk

### Locking/Unlocking Screens
- **Lock All**: Toolbar button to lock all connected clients
- **Unlock All**: Toolbar button to unlock all clients
- **Individual Lock**: Right-click client thumbnail for per-client control

### Domain Filtering
1. Click **Policy** → **Domain Filter**
2. Choose mode: **Blacklist** or **Whitelist**
3. Add domains to block/allow
4. Click **Apply** to push rules to all clients

### Application Control
1. Click **Policy** → **Application Filter**
2. Select filter mode
3. Add application paths or process names
4. Rules are applied immediately

## 🛠️ Technical Details

### Screenshot Capture
- Uses `GetDeviceCaps(DESKTOPHORZRES/DESKTOPVERTRES)` for DPI-aware resolution
- Captures raw RGBA pixel data (4 bytes per pixel)
- Automatic resolution detection from data size
- Support for common resolutions: 1920x1080, 1600x900, 1366x768, 1280x720

### Network Protocol
Messages are JSON objects with newline delimiters:

```json
{
  "type": "SCREENSHOT_REQUEST",
  "source": "master",
  "destination": "client-01",
  "timestamp": 1234567890,
  "payload": {}
}
```

### Command Types
- `HELLO` - Client handshake
- `PING` - Heartbeat
- `SCREENSHOT_REQUEST` - Request screenshot
- `SCREENSHOT_DATA` - Screenshot response
- `SCREEN_LOCK` / `SCREEN_UNLOCK` - Input control
- `DOMAIN_BLOCK` / `DOMAIN_ALLOW` - Network filtering
- `APP_BLOCK` / `APP_ALLOW` - Application control
- `POWER_CONTROL` - Shutdown/reboot/hibernate

## 🐛 Known Issues

- Input locking requires administrator privileges
- Domain filtering modifies system hosts file (admin required)
- Multi-monitor support captures primary screen only in some cases

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built with Qt 6 Framework
- Uses nlohmann/json for JSON parsing
- Windows API for platform-specific features

## 📞 Support

For issues and questions:
- Create an issue on GitHub
- Check existing issues for solutions
- Review the documentation

---

**Note**: This software is intended for educational environments. Ensure you have proper authorization before deploying on any network.
