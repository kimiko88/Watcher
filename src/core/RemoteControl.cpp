#include "cms/RemoteControl.h"
#include "cms/Logger.h"
#include <numeric>
#include <algorithm>

namespace cms {
namespace control {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

RemoteControl::RemoteControl(const std::string& client_id)
    : client_id_(client_id)
    , is_connected_(false)
{
    // Reserve space for latency history (e.g., last 10 samples)
    latency_history_.reserve(10);
}

RemoteControl::~RemoteControl() {
    stopSession();
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

bool RemoteControl::startSession() {
    if (is_connected_) {
        LOG_WARNING("Remote session already active for client: " + client_id_);
        return true; // Already started
    }

    LOG_INFO("Starting remote session for client: " + client_id_);
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    is_connected_ = true;
    
    // Reset stats
    stats_ = StreamStats();
    latency_history_.clear();
    
    return true;
}

bool RemoteControl::stopSession() {
    if (!is_connected_) {
        return true;
    }

    LOG_INFO("Stopping remote session for client: " + client_id_);
    is_connected_ = false;
    return true;
}

bool RemoteControl::isConnected() const {
    return is_connected_;
}

// ============================================================================
// VIEW
// ============================================================================

cms::capture::ScreenFrame RemoteControl::getRemoteScreen() {
    if (!is_connected_) {
        return cms::capture::ScreenFrame{};
    }

    // Placeholder: In real implementation, this would fetch from Network layer buffer
    // For now, return empty frame or last cached frame
    return cms::capture::ScreenFrame{};
}

// ============================================================================
// INPUT CONTROL
// ============================================================================

bool RemoteControl::sendMouseEvent(const RemoteMouseEvent& event) {
    if (!is_connected_) return false;
    
    // Logic to serialize and send over network would go here
    // For Core logic, we just confirm we are in valid state to send
    return true;
}

bool RemoteControl::sendKeyboardEvent(const RemoteKeyboardEvent& event) {
    if (!is_connected_) return false;
    
    return true;
}

// ============================================================================
// STATS
// ============================================================================

StreamStats RemoteControl::getLiveStreamStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

uint32_t RemoteControl::getLatency() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_.latency_ms;
}

void RemoteControl::updateStreamStats(uint32_t bytes_received, uint32_t round_trip_time_ms) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Update latency moving average
    if (latency_history_.size() >= 10) {
        latency_history_.erase(latency_history_.begin());
    }
    latency_history_.push_back(round_trip_time_ms);
    
    uint64_t sum = std::accumulate(latency_history_.begin(), latency_history_.end(), 0ULL);
    stats_.latency_ms = static_cast<uint32_t>(sum / latency_history_.size());
    
    // Bandwidth calculation would need time delta
    // For now, simplistic update or placeholder
}

void RemoteControl::reportFrameReceived() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.frames_received++;
}

void RemoteControl::recalculateStats() {
    // Internal helper if needed
}

} // namespace control
} // namespace cms
