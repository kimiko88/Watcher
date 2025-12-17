#ifndef CMS_INPUT_LOCK_MANAGER_H
#define CMS_INPUT_LOCK_MANAGER_H

#include "Common.h"
#include "Platform.h"
#include <mutex>

namespace cms {
namespace input {

// ============================================================================
// LOCK LEVEL ENUM
// ============================================================================

enum class LockLevel {
    LOCK_NONE,           // Everything unlocked
    LOCK_KEYBOARD_ONLY,  // Only keyboard locked, mouse free
    LOCK_MOUSE_ONLY,     // Only mouse locked, keyboard free
    LOCK_BOTH            // All input blocked
};

// ============================================================================
// INPUT LOCK MANAGER
// ============================================================================

class InputLockManager {
public:
    // Constructor - requires input control interface
    // Throws std::invalid_argument if input_control is null
    explicit InputLockManager(platform::IInputControl* input_control);
    
    // Destructor - ensures everything is unlocked
    ~InputLockManager();
    
    // Basic lock/unlock operations
    // Returns true if operation succeeded
    // May return false if admin privileges are required
    bool lockKeyboard();
    bool unlockKeyboard();
    bool lockMouse();
    bool unlockMouse();
    
    // State queries
    // Thread-safe
    bool isKeyboardLocked() const;
    bool isMouseLocked() const;
    
    // Level management
    // Sets the lock level and applies corresponding locks
    // Returns true if operation succeeded
    bool setLockLevel(LockLevel level);
    
    // Get current lock level
    // Thread-safe
    LockLevel getLockLevel() const;
    
private:
    platform::IInputControl* input_control_;
    mutable std::mutex mutex_;
    LockLevel current_level_;
    bool keyboard_locked_;
    bool mouse_locked_;
    
    // Helper to apply lock level
    bool applyLockLevel(LockLevel level);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Convert LockLevel to string
inline const char* lockLevelToString(LockLevel level) {
    switch (level) {
        case LockLevel::LOCK_NONE: return "LOCK_NONE";
        case LockLevel::LOCK_KEYBOARD_ONLY: return "LOCK_KEYBOARD_ONLY";
        case LockLevel::LOCK_MOUSE_ONLY: return "LOCK_MOUSE_ONLY";
        case LockLevel::LOCK_BOTH: return "LOCK_BOTH";
        default: return "Unknown";
    }
}

} // namespace input
} // namespace cms

#endif // CMS_INPUT_LOCK_MANAGER_H
