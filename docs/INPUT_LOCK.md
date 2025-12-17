# Input Lock Manager - Documentation

## Overview

The **InputLockManager** provides a high-level interface for managing input locking (keyboard and mouse) on student machines. It builds upon the platform-specific `IInputControl` interface to provide state tracking, thread safety, and unified lock levels.

## Architecture

```
┌─────────────────────────────────────────┐
│           InputLockManager              │
│       (Thread-Safe State Manager)       │
├─────────────────────────────────────────┤
│                                         │
│   setLockLevel(LOCK_BOTH) ──┐           │
│                             │           │
│                             ▼           │
│                    ┌─────────────────┐  │
│                    │ Platform Layer  │  │
│                    │ IInputControl   │  │
│                    └─────────────────┘  │
│                             │           │
│           ┌─────────────────┴───────────┐
│           ▼                             ▼
│    lockKeyboard()                  lockMouse()
│  (BlockInput/Hooks)            (BlockInput/Hooks)
│                                         │
└─────────────────────────────────────────┘
```

## lockLevel Enum

The manager uses a simple enum to define the desired lock state:

```cpp
enum class LockLevel {
    LOCK_NONE,           // Everything unlocked
    LOCK_KEYBOARD_ONLY,  // Mouse free, keyboard locked
    LOCK_MOUSE_ONLY,     // Keyboard free, mouse locked
    LOCK_BOTH            // Total input block
};
```

## Usage

### Basic Initialization

```cpp
#include "cms/InputLockManager.h"
#include "cms/Platform.h"

using namespace cms::input;
using namespace cms::platform;

// Initialize
auto platform = getPlatformInstance();
InputLockManager lockManager(platform.get());
```

### Locking Input

Most common usage is via `setLockLevel`:

```cpp
// Lock everything (e.g., during exam validation)
if (lockManager.setLockLevel(LockLevel::LOCK_BOTH)) {
    std::cout << "Input locked successfully" << std::endl;
} else {
    std::cerr << "Failed to lock input (Admin required?)" << std::endl;
}

// Lock only keyboard (allow mouse for read-only navigation)
lockManager.setLockLevel(LockLevel::LOCK_KEYBOARD_ONLY);

// Unlock everything
lockManager.setLockLevel(LockLevel::LOCK_NONE);
```

### Querying State

The manager tracks state internally, so queries are fast and thread-safe:

```cpp
if (lockManager.isKeyboardLocked()) {
    // Show "Locked" overlay
}

LockLevel current = lockManager.getLockLevel();
```

## Security & Permissions

> [!CAUTION]
> **Administrator Privileges Required**

Input locking is a sensitive system operation. On most platforms, it requires elevated privileges:

- **Windows**: Requires **Administrator** access to call `BlockInput`. If run as a standard user, calls will fail and return `false`.
- **Linux**: Requires **root** or specific udev rules to grab input devices.
- **macOS**: Requires **Accessibility** permissions in System Preferences.

### Safety Mechanisms

1. **Auto-Unlock**: The destructor automatically unlocks everything to prevent accidental permanent lockouts if the service crashes or stops.
2. **Idempotency**: Calling `lockKeyboard()` when already locked is safe and does nothing.
3. **Thread Safety**: All methods are protected by a mutex, allowing concurrent access from multiple threads (e.g., heartbeat thread and command processing thread).

## Platform Implementation Details

- **Windows**: Uses `BlockInput` WinAPI. This blocks *physical* input events but may not stop simulated input from other admin processes. Does NOT block Ctrl+Alt+Del (system security feature).
- **Linux**: (Planned) Uses `evdev` to grab exclusive access to input devices.
- **macOS**: (Planned) Uses `Quartz Event Services` (CGEventTap) to filter input events.

## Limitations

1. **System Hotkeys**:
   - **Ctrl+Alt+Del** cannot be blocked by software on Windows (SAS - Secure Attention Sequence).
   - **Win+L** (Lock Screen) usually cannot be blocked.
   
2. **Visual Feedback**:
   - This manager only handles the *mechanism* of locking.
   - It does NOT draw a "Locked" overlay screen. That must be handled by a separate UI component.

3. **Remote Input**:
   - Does not necessarily block input injected by remote desktop software (TeamViewer, RDP), which is desirable for admin control.
