# Classroom Control (CMS) - C++ Classroom Management System

A cross-platform classroom management system built with modern C++17, designed to provide efficient control and monitoring of classroom environments.

## Features

- **Cross-Platform**: Supports Windows, Linux, and macOS
- **Modern C++17**: Utilizes modern C++ features and best practices
- **Modular Architecture**: Separates concerns into core, client, and master components
- **Robust Testing**: Comprehensive test coverage with Google Test
- **Flexible Configuration**: Simple configuration management system
- **Professional Logging**: Multi-level logging with thread safety

## Project Structure

```
Watcher/
├── CMakeLists.txt              # Root build configuration
├── README.md                   # This file
├── .gitignore                  # Git ignore rules
├── include/
│   └── cms/
│       ├── Common.h            # Shared types and constants
│       ├── Logger.h            # Logging interface
│       └── Config.h            # Configuration management
├── src/
│   ├── core/                   # Business logic library
│   ├── client/                 # Client application
│   ├── master/                 # Master/server application
│   ├── network/                # Networking layer (placeholder)
│   └── platform/               # Platform-specific code (placeholder)
├── tests/
│   ├── unit/                   # Unit tests
│   └── integration/            # Integration tests
└── docs/
    └── ARCHITECTURE.md         # Architecture documentation
```

## Requirements

- **CMake** 3.14 or higher
- **C++17** compatible compiler:
  - Windows: Visual Studio 2017 or later / MinGW-w64
  - Linux: GCC 7+ or Clang 5+
  - macOS: Xcode 10+ (Apple Clang)
- **Internet connection** (first build only, for downloading Google Test)

## Building

### Windows (Visual Studio)

```powershell
# Configure
cmake -B build -G "Visual Studio 16 2019"

# Build Debug
cmake --build build --config Debug

# Build Release
cmake --build build --config Release
```

### Windows (MinGW)

```powershell
# Configure
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build
```

### Linux / macOS

```bash
# Configure Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Or configure Release build
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

## Running Tests

### All Tests

```bash
cd build
ctest --output-on-failure
```

### Verbose Output

```bash
cd build
ctest -V
```

### Run Specific Test Suite

```bash
# Unit tests only
./build/tests/cms_unit_tests

# Integration tests only
./build/tests/cms_integration_tests
```

## Running Applications

### Client Application

```bash
# Windows
.\build\src\client\Debug\cms_client.exe

# Linux/macOS
./build/src/client/cms_client
```

### Master Application

```bash
# Windows
.\build\src\master\Debug\cms_master.exe

# Linux/macOS
./build/src/master/cms_master
```

## Development

### Adding New Features

1. **Create header files** in `include/cms/`
2. **Implement in** `src/core/` for shared code
3. **Write tests first** (TDD approach) in `tests/unit/`
4. **Update CMakeLists.txt** if adding new files
5. **Run tests** to verify

### Build Types

- **Debug**: Full debugging symbols, no optimization (`-g -O0`)
- **Release**: Optimized for performance (`-O3`, `-DNDEBUG`)

### Compiler Warnings

- **MSVC**: `/W4` (Debug), `/W3` (Release)
- **GCC/Clang**: `-Wall -Wextra` (Debug)

## Components

### Core Library (`cms_core`)

Static library containing:
- Common types and constants
- Logger implementation
- Configuration management
- Core business logic

### Client Application (`cms_client`)

Student/client-side application that connects to the master server.

### Master Application (`cms_master`)

Teacher/server-side application that manages and monitors clients.

## Configuration

The system uses a simple key-value configuration system:

```cpp
auto& config = cms::Config::Instance();
config.Set("server.port", "8080");
auto port = config.Get("server.port");
```

## Logging

Multi-level logging with thread safety:

```cpp
LOG_DEBUG("Debug information");
LOG_INFO("General information");
LOG_WARNING("Warning message");
LOG_ERROR("Error occurred");
```

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]

## Authors

- [Your name/team here]

## Acknowledgments

- Google Test framework for testing infrastructure
- CMake for cross-platform build system
