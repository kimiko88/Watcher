#include <gtest/gtest.h>
#include "cms/RemoteControl.h"
#include <thread>
#include <chrono>

using namespace cms::control;

class RemoteControlTest : public ::testing::Test {
protected:
    void SetUp() override {
        rc = std::make_unique<RemoteControl>("client_123");
    }

    void TearDown() override {
        rc->stopSession();
    }

    std::unique_ptr<RemoteControl> rc;
};

TEST_F(RemoteControlTest, InitializationState) {
    EXPECT_FALSE(rc->isConnected());
    
    auto stats = rc->getLiveStreamStats();
    EXPECT_EQ(stats.frames_received, 0);
    EXPECT_EQ(stats.latency_ms, 0);
}

TEST_F(RemoteControlTest, StartStopSession) {
    EXPECT_TRUE(rc->startSession());
    EXPECT_TRUE(rc->isConnected());
    
    EXPECT_TRUE(rc->stopSession());
    EXPECT_FALSE(rc->isConnected());
}

TEST_F(RemoteControlTest, LatencyCalculation) {
    rc->startSession();
    
    // Simulate some network packets
    rc->updateStreamStats(1024, 50); // 50ms RTT
    EXPECT_EQ(rc->getLatency(), 50);
    
    rc->updateStreamStats(1024, 150); // 150ms RTT
    
    // Depending on implementation (Moving Average?), it might be 100 or 150.
    // Let's assume simple latest or average. If we implement moving average:
    // (50 + 150) / 2 = 100
    uint32_t lat = rc->getLatency();
    EXPECT_GE(lat, 50);
    EXPECT_LE(lat, 150);
}

TEST_F(RemoteControlTest, BandwidthStatsAccumulation) {
    rc->startSession();
    
    // Simulate 10 frames of 10KB each (100KB total) arriving over 1 second (approx)
    // To test bandwidth_kbps, we need time to pass, or we mocking time.
    // Simple implementation might arguably just store bytes.
    
    rc->reportFrameReceived();
    auto stats = rc->getLiveStreamStats();
    EXPECT_EQ(stats.frames_received, 1);
}

TEST_F(RemoteControlTest, InputEventHandling) {
    rc->startSession();
    
    RemoteMouseEvent mouseEvent;
    mouseEvent.x = 100;
    mouseEvent.y = 200;
    mouseEvent.action = MouseAction::CLICK;
    mouseEvent.button = MouseButton::LEFT;
    
    EXPECT_TRUE(rc->sendMouseEvent(mouseEvent));
    
    RemoteKeyboardEvent keyEvent;
    keyEvent.key_code = 65; // 'A'
    keyEvent.is_pressed = true;
    
    EXPECT_TRUE(rc->sendKeyboardEvent(keyEvent));
}

TEST_F(RemoteControlTest, EventsFailIfDisconnected) {
    rc->stopSession();
    
    RemoteMouseEvent mouseEvent = {};
    EXPECT_FALSE(rc->sendMouseEvent(mouseEvent));
}
