# Watcher - Service Architecture

## Quick Start for Developers

### Prerequisites
- Visual Studio 2022 with C++17 support
- CMake 3.20+
- Qt 6.5+ (for GUI)
- Windows SDK 10.0+

### Building

```batch
# Clean build
rmdir /s /q build
mkdir build
cd build

# Configure (set Qt path if needed)
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.10.1\msvc2022_64"

# Build all targets
cmake --build . --config Release

# Or build specific targets
cmake --build . --target cms_core --config Release
cmake --build . --target cms_client_service --config Release
cmake --build . --target cms_client_worker --config Release
cmake --build . --target cms_master_service --config Release
cmake --build . --target cms_master --config Release
```

### Architecture Overview

Watcher now uses a **Veyon-like service architecture**:

**Client Side**:
```
Windows Service (cms_client_service.exe)
    ├─ Manages worker process
    ├─ IPC via Named Pipe
    └─> Worker Process (cms_client_worker.exe)
        └─ Connects to master via TCP
```

**Master Side**:
```
Windows Service (cms_master_service.exe)
    ├─ Runs MasterServer
    ├─ IPC via Named Pipe
    └─> GUI Process (cms_master.exe --ipc)
        └─ Optional Qt interface
```

### Installation

**Client** (on student machines):
```batch
cd build\Release
..\..\scripts\install_client_service.bat
```

**Master** (on teacher machine):
```batch
cd build\Release
..\..\scripts\install_master_service.bat
```

### Service Management

```batch
# Check status
sc query WatcherClientService
sc query WatcherMasterService

# Start/Stop
sc start WatcherClientService
sc stop WatcherClientService

# View logs
type C:\Users\Public\cms_service_log.txt
type C:\Users\Public\cms_worker_debug.txt
```

### Project Structure

```
Watcher/
├── include/cms/
│   ├── IPCProtocol.h          # IPC message definitions
│   ├── IPCChannel.h           # Named Pipe wrapper
│   ├── ServiceLauncher.h      # Client worker launcher
│   ├── MasterService.h        # Master service manager
│   ├── ClientService.h        # Client logic
│   └── MasterServer.h         # Server logic
├── src/
│   ├── core/
│   │   ├── IPCProtocol.cpp
│   │   └── IPCChannel.cpp
│   ├── client/
│   │   ├── ServiceLauncher.cpp
│   │   ├── client_service_main.cpp  # Service entry
│   │   └── main.cpp                 # Worker entry
│   └── master/
│       ├── MasterService.cpp
│       ├── master_service_main.cpp  # Service entry
│       └── main.cpp                 # GUI entry
├── scripts/
│   ├── install_client_service.bat
│   ├── uninstall_client_service.bat
│   ├── install_master_service.bat
│   └── uninstall_master_service.bat
└── tests/
    └── ipc/
```

### IPC Communication

**Named Pipes**:
- Client: `\\.\pipe\WatcherClientService`
- Master: `\\.\pipe\WatcherMasterService`

**Message Types**:
- `PROCESS_READY` - Worker/GUI ready
- `PROCESS_STATUS` - Heartbeat with status
- `SERVICE_CONFIG` - Configuration from service
- `SERVICE_SHUTDOWN` - Graceful shutdown request
- `SERVER_STATE` - Master server state update
- `EXECUTE_COMMAND` - Command from GUI

### Troubleshooting

**Build Issues**:
See [build_troubleshooting.md](file:///C:/Users/chimi/.gemini/antigravity/brain/30a9bebd-077f-40ab-b083-aa528789c37b/build_troubleshooting.md)

**Service Won't Start**:
1. Check logs in `C:\Users\Public\`
2. Verify executables exist
3. Run as Administrator

**Worker Crashes**:
- Check `cms_service_log.txt` for restart attempts
- Service auto-restarts with exponential backoff
- Max 5 consecutive restarts before giving up

### Development

**Adding New IPC Messages**:
1. Add message type to `IPCProtocol.h`
2. Update `IPCMessageTypeToString()`
3. Handle in service/worker message handler

**Testing IPC**:
```cpp
// In tests/ipc/test_named_pipe.cpp
TEST(IPCTest, BasicCommunication) {
  // Server
  NamedPipeServer server("TestPipe");
  server.Start();
  
  // Client
  NamedPipeClient client("TestPipe");
  client.Connect();
  
  // Send message
  IPCMessage msg = IPCMessage::Create(IPCMessageType::PROCESS_READY);
  client.SendIPCMessage(msg);
}
```

### Contributing

1. Create feature branch
2. Implement changes
3. Add tests
4. Update documentation
5. Submit PR

### License

[Your License]

---

For detailed architecture information, see [walkthrough.md](file:///C:/Users/chimi/.gemini/antigravity/brain/30a9bebd-077f-40ab-b083-aa528789c37b/walkthrough.md)
