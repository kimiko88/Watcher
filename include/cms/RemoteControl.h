#ifndef CMS_REMOTE_CONTROL_H
#define CMS_REMOTE_CONTROL_H

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <memory>
#include "cms/ScreenCapture.h" // For ScreenFrame

namespace cms {
namespace control {

// ============================================================================
// DATA STRUCTURES
// ============================================================================

struct StreamStats {
    float fps = 0.0f;
    float bandwidth_kbps = 0.0f;
    float compression_ratio = 1.0f;
    uint32_t latency_ms = 0;
    uint64_t frames_received = 0;
    uint64_t frames_dropped = 0;
};

enum class MouseButton {
    LEFT,
    RIGHT,
    MIDDLE
};

enum class MouseAction {
    MOVE,
    PRESS,
    RELEASE,
    CLICK,
    DOUBLE_CLICK
};

struct RemoteMouseEvent {
    int x;
    int y;
    MouseButton button;
    MouseAction action;
    int wheel_delta; // For scroll
};

struct RemoteKeyboardEvent {
    uint32_t key_code;
    bool is_pressed;
    bool shift_down;
    bool ctrl_down;
    bool alt_down;
};

// ============================================================================
// REMOTE CONTROL CLASS
// ============================================================================

class RemoteControl {
public:
    // Constructor
    // master_server ptr would normally go here, but for Core logic we might keep it decoupled
    // for now we just pass client ID.
    explicit RemoteControl(const std::string& client_id);
    virtual ~RemoteControl();

    // Session Management
    bool startSession();
    bool stopSession();
    bool isConnected() const;

    // View
    // Returns the latest frame from the remote client
    cms::capture::ScreenFrame getRemoteScreen();

    // Input Control
    bool sendMouseEvent(const RemoteMouseEvent& event);
    bool sendKeyboardEvent(const RemoteKeyboardEvent& event);

    // Stats
    StreamStats getLiveStreamStats() const;
    uint32_t getLatency() const;

    // Internal / Simulation (Public for testing mostly, or used by network layer)
    void updateStreamStats(uint32_t bytes_received, uint32_t round_trip_time_ms);
    void reportFrameReceived();

private:
    std::string client_id_;
    std::atomic<bool> is_connected_;
    
    // Stats
    mutable std::mutex stats_mutex_;
    StreamStats stats_;
    
    // Internal tracking for simple moving average of latency
    std::vector<uint32_t> latency_history_;
    
    // Helper to update stats
    void recalculateStats();
};

} // namespace control
} // namespace cms

#endif // CMS_REMOTE_CONTROL_H
