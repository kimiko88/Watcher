#include "cms/InputLockManager.h"
#include "cms/Logger.h"
#include <stdexcept>

namespace cms {
namespace input {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

InputLockManager::InputLockManager(platform::IInputControl* input_control)
    : input_control_(input_control)
    , current_level_(LockLevel::LOCK_NONE)
    , keyboard_locked_(false)
    , mouse_locked_(false)
{
    if (!input_control_) {
        throw std::invalid_argument("InputControl cannot be null");
    }
    
    LOG_INFO("InputLockManager initialized");
}

InputLockManager::~InputLockManager() {
    // Ensure everything is unlocked on destruction
    setLockLevel(LockLevel::LOCK_NONE);
    LOG_INFO("InputLockManager destroyed");
}

// ============================================================================
// BASIC LOCK / UNLOCK
// ============================================================================

bool InputLockManager::lockKeyboard() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_DEBUG("Locking keyboard...");
    
    bool result = input_control_->lockKeyboard();
    if (result) {
        keyboard_locked_ = true;
        LOG_INFO("Keyboard locked");
    } else {
        LOG_WARNING("Failed to lock keyboard (admin required?)");
    }
    
    return result;
}

bool InputLockManager::unlockKeyboard() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_DEBUG("Unlocking keyboard...");
    
    bool result = input_control_->unlockKeyboard();
    if (result) {
        keyboard_locked_ = false;
        LOG_INFO("Keyboard unlocked");
    } else {
        LOG_WARNING("Failed to unlock keyboard");
    }
    
    return result;
}

bool InputLockManager::lockMouse() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_DEBUG("Locking mouse...");
    
    bool result = input_control_->lockMouse();
    if (result) {
        mouse_locked_ = true;
        LOG_INFO("Mouse locked");
    } else {
        LOG_WARNING("Failed to lock mouse (admin required?)");
    }
    
    return result;
}

bool InputLockManager::unlockMouse() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_DEBUG("Unlocking mouse...");
    
    bool result = input_control_->unlockMouse();
    if (result) {
        mouse_locked_ = false;
        LOG_INFO("Mouse unlocked");
    } else {
        LOG_WARNING("Failed to unlock mouse");
    }
    
    return result;
}

// ============================================================================
// STATE QUERIES
// ============================================================================

bool InputLockManager::isKeyboardLocked() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return keyboard_locked_;
}

bool InputLockManager::isMouseLocked() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mouse_locked_;
}

// ============================================================================
// LOCK LEVEL MANAGEMENT
// ============================================================================

bool InputLockManager::setLockLevel(LockLevel level) {
    LOG_INFO(std::string("Setting lock level to: ") + lockLevelToString(level));
    
    bool result = applyLockLevel(level);
    
    if (result) {
        std::lock_guard<std::mutex> lock(mutex_);
        current_level_ = level;
        LOG_INFO("Lock level set successfully");
    } else {
        LOG_WARNING("Failed to set lock level");
    }
    
    return result;
}

LockLevel InputLockManager::getLockLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_level_;
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

bool InputLockManager::applyLockLevel(LockLevel level) {
    bool success = true;
    
    switch (level) {
        case LockLevel::LOCK_NONE:
            // Unlock everything
            if (!unlockKeyboard()) success = false;
            if (!unlockMouse()) success = false;
            break;
            
        case LockLevel::LOCK_KEYBOARD_ONLY:
            // Lock keyboard, unlock mouse
            if (!lockKeyboard()) success = false;
            if (!unlockMouse()) success = false;
            break;
            
        case LockLevel::LOCK_MOUSE_ONLY:
            // Lock mouse, unlock keyboard
            if (!unlockKeyboard()) success = false;
            if (!lockMouse()) success = false;
            break;
            
        case LockLevel::LOCK_BOTH:
            // Lock everything
            if (!lockKeyboard()) success = false;
            if (!lockMouse()) success = false;
            break;
            
        default:
            LOG_ERROR("Unknown lock level");
            success = false;
            break;
    }
    
    return success;
}

} // namespace input
} // namespace cms
