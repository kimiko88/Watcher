# Classroom Control (CMS) Architecture

## Overview

Classroom Control (CMS) is a cross-platform classroom management system designed with a client-server architecture. The system allows teachers (master) to monitor and control student computers (clients) in a classroom environment.

## Design Principles

1. **Modularity**: Clear separation of concerns into distinct modules
2. **Cross-Platform**: Single codebase for Windows, Linux, and macOS
3. **Testability**: TDD approach with comprehensive test coverage
4. **Extensibility**: Easy to add new features and components
5. **Performance**: Modern C++17 for optimal performance

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                     CMS System                           │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────┐              ┌──────────────┐        │
│  │   Master     │◄────────────►│   Client     │        │
│  │ Application  │   Network    │ Application  │        │
│  └──────┬───────┘              └──────┬───────┘        │
│         │                              │                │
│         └──────────┬───────────────────┘                │
│                    │                                    │
│         ┌──────────▼──────────┐                        │
│         │    CMS Core         │                        │
│         │   (Static Library)  │                        │
│         └──────────┬──────────┘                        │
│                    │                                    │
│    ┌───────────────┼───────────────┐                  │
│    │               │               │                  │
│ ┌──▼───┐      ┌───▼────┐    ┌────▼─────┐            │
│ │Logger│      │Config  │    │ Common   │            │
│ └──────┘      └────────┘    └──────────┘            │
│                                                        │
└────────────────────────────────────────────────────────┘
```

## Component Details

### 1. Core Library (`cms_core`)

**Purpose**: Provides shared functionality for both client and master applications.

**Responsibilities**:
- System initialization and shutdown
- Common types and constants
- Logging infrastructure
- Configuration management
- Business logic

**Key Files**:
- `include/cms/Common.h` - Common types, constants, and platform detection
- `include/cms/Logger.h` - Thread-safe logging system
- `include/cms/Config.h` - Configuration key-value store
- `src/core/Core.cpp` - Core implementation

**Design Patterns**:
- **Singleton**: Logger and Config use singleton pattern for global access
- **Facade**: Core provides simplified interface to complex subsystems

### 2. Client Application (`cms_client`)

**Purpose**: Runs on student computers, receives commands from master.

**Responsibilities**:
- Connect to master server
- Execute received commands
- Report status back to master
- Monitor local system resources

**Key Files**:
- `src/client/main.cpp` - Client entry point

**Future Enhancements**:
- Screen capture and streaming
- Input control (keyboard/mouse locking)
- Application monitoring and control
- File transfer capabilities

### 3. Master Application (`cms_master`)

**Purpose**: Runs on teacher computer, manages all client connections.

**Responsibilities**:
- Accept client connections
- Send commands to clients
- Monitor client status
- Display client screens
- Manage classroom sessions

**Key Files**:
- `src/master/main.cpp` - Master entry point

**Future Enhancements**:
- Web-based dashboard
- Student grouping
- Screen broadcasting
- Session recording
- Analytics and reporting

### 4. Network Layer (Placeholder)

**Purpose**: Handle all network communication between master and clients.

**Planned Features**:
- TCP/IP communication
- Message serialization/deserialization
- Connection management
- Heartbeat mechanism
- Encryption for security

**Key Files**:
- `src/network/Network.cpp` - Network implementation

**Technology Considerations**:
- **Boost.Asio**: For cross-platform networking
- **Protocol Buffers**: For message serialization
- **OpenSSL**: For secure communication

### 5. Platform Layer (Placeholder)

**Purpose**: Isolate platform-specific code for better maintainability.

**Responsibilities**:
- OS-specific system calls
- Window management
- Process control
- Hardware interaction

**Key Files**:
- `src/platform/Platform.cpp` - Platform-specific code

**Platform-Specific Features**:
- **Windows**: Win32 API, DirectX for screen capture
- **Linux**: X11/Wayland, /proc filesystem
- **macOS**: Cocoa APIs, Core Graphics

## Data Flow

### Client Startup Flow

```
1. Client starts
2. Load configuration
3. Initialize core systems
4. Connect to master server
5. Wait for commands
6. Process commands
7. Send responses
```

### Master Startup Flow

```
1. Master starts
2. Load configuration
3. Initialize core systems
4. Start network server
5. Accept client connections
6. Monitor connected clients
7. Send commands to clients
```

## Build System

The project uses **CMake** as the build system generator:

- **Root CMakeLists.txt**: Defines project settings, platform detection, Google Test integration
- **Module CMakeLists.txt**: Each module has its own build configuration
- **Test CMakeLists.txt**: Configures test executables and discovery

### Build Configurations

1. **Debug**:
   - Full debug symbols (`-g`)
   - No optimization (`-O0`)
   - Additional warnings (`-Wall -Wextra`)
   - Suitable for development

2. **Release**:
   - Maximum optimization (`-O3`)
   - No debug symbols
   - `NDEBUG` defined
   - Suitable for production

## Testing Strategy

Following **Test-Driven Development (TDD)**:

### Unit Tests (`tests/unit/`)

- Test individual components in isolation
- Mock external dependencies
- Fast execution
- High coverage

**Current Tests**:
- `test_common.cpp`: Common types and constants
- `test_logger.cpp`: Logger functionality

### Integration Tests (`tests/integration/`)

- Test component interactions
- Verify build and linking
- End-to-end scenarios

**Current Tests**:
- `test_build.cpp`: Compilation and integration verification

### Test Execution

```bash
# All tests
ctest --output-on-failure

# Specific test
./build/tests/cms_unit_tests --gtest_filter=CommonTest.*
```

## Platform Support

### Windows

- **Compilers**: MSVC 2017+, MinGW-w64
- **Platform Macro**: `CMS_PLATFORM_WINDOWS`
- **Threading**: Windows threads / std::thread

### Linux

- **Compilers**: GCC 7+, Clang 5+
- **Platform Macro**: `CMS_PLATFORM_LINUX`
- **Threading**: pthreads / std::thread

### macOS

- **Compilers**: Apple Clang (Xcode 10+)
- **Platform Macro**: `CMS_PLATFORM_MACOS`
- **Threading**: pthreads / std::thread

## Security Considerations

### Current

- Thread-safe logging to prevent race conditions
- Input validation in configuration system

### Planned

- **Authentication**: Verify client identity
- **Encryption**: TLS/SSL for network communication
- **Authorization**: Role-based access control
- **Audit Logging**: Track all administrative actions
- **Sandboxing**: Limit client capabilities

## Performance Considerations

- **Static Library**: Core is compiled once, linked multiple times
- **Modern C++17**: Move semantics, constexpr for efficiency
- **Minimal Dependencies**: Reduce overhead
- **Efficient Logging**: Thread-safe with minimal locking

## Future Roadmap

### Phase 1: Foundation (Current)
- ✅ Project structure
- ✅ Build system
- ✅ Core components
- ✅ Basic testing

### Phase 2: Networking
- ⬜ Network protocol design
- ⬜ Client-server communication
- ⬜ Connection management
- ⬜ Message serialization

### Phase 3: Core Features
- ⬜ Screen monitoring
- ⬜ Remote control
- ⬜ Application control
- ⬜ File transfer

### Phase 4: Advanced Features
- ⬜ Web dashboard
- ⬜ Session recording
- ⬜ Analytics
- ⬜ Plugin system

### Phase 5: Production Ready
- ⬜ Security hardening
- ⬜ Performance optimization
- ⬜ Documentation
- ⬜ Installer/Packaging

## Coding Standards

- **C++ Standard**: C++17
- **Naming**:
  - Classes: `PascalCase`
  - Functions: `PascalCase`
  - Variables: `camelCase`
  - Constants: `UPPER_CASE`
  - Namespaces: `lowercase`
- **Formatting**: Consistent indentation (4 spaces)
- **Comments**: Doxygen-style for public APIs
- **Error Handling**: Return codes via `StatusCode` enum

## Dependencies

### Build-time
- CMake 3.14+
- C++17 compiler

### Runtime
- None (currently)

### Test
- Google Test 1.12.1 (fetched automatically)

## References

- [CMake Documentation](https://cmake.org/documentation/)
- [Google Test](https://github.com/google/googletest)
- [C++17 Standard](https://en.cppreference.com/w/cpp/17)
