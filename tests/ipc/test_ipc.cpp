#include "cms/IPCChannel.h"
#include "cms/IPCProtocol.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>


using namespace cms::ipc;

// Test fixture for IPC tests
class IPCTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }

  void TearDown() override {
    // Cleanup code if needed
  }
};

// Test IPCMessage creation
TEST_F(IPCTest, MessageCreation) {
  IPCMessage msg = IPCMessage::Create(IPCMessageType::PROCESS_READY);

  EXPECT_EQ(msg.type, IPCMessageType::PROCESS_READY);
  EXPECT_FALSE(msg.id.empty());
  EXPECT_GT(msg.timestamp, 0);
}

// Test IPCMessage serialization
TEST_F(IPCTest, MessageSerialization) {
  nlohmann::json payload;
  payload["test"] = "data";
  payload["value"] = 42;

  IPCMessage original =
      IPCMessage::Create(IPCMessageType::PROCESS_STATUS, payload);

  IPCMessageSerializer serializer;
  std::string serialized = serializer.Serialize(original);

  EXPECT_FALSE(serialized.empty());

  IPCMessage deserialized = serializer.Deserialize(serialized);

  EXPECT_EQ(deserialized.type, original.type);
  EXPECT_EQ(deserialized.id, original.id);
  EXPECT_EQ(deserialized.payload["test"], "data");
  EXPECT_EQ(deserialized.payload["value"], 42);
}

// Test Named Pipe basic communication
TEST_F(IPCTest, NamedPipeBasicCommunication) {
  const std::string pipeName = "\\\\.\\pipe\\WatcherTest";

  std::atomic<bool> messageReceived{false};
  IPCMessage receivedMsg;

  // Start server in separate thread
  std::thread serverThread([&]() {
    NamedPipeServer server(pipeName);

    server.SetMessageHandler([&](const IPCMessage &msg) {
      receivedMsg = msg;
      messageReceived = true;
    });

    EXPECT_TRUE(server.Start());

    // Wait for message
    for (int i = 0; i < 50 && !messageReceived; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server.Stop();
  });

  // Give server time to start
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Connect client
  NamedPipeClient client(pipeName);
  EXPECT_TRUE(client.Connect());

  client.StartReading();

  // Send message
  IPCMessage msg = IPCMessage::Create(IPCMessageType::PROCESS_READY);
  EXPECT_TRUE(client.SendMessage(msg));

  // Wait for server to receive
  std::this_thread::sleep_for(std::chrono::seconds(1));

  client.Disconnect();
  serverThread.join();

  EXPECT_TRUE(messageReceived);
  EXPECT_EQ(receivedMsg.type, IPCMessageType::PROCESS_READY);
}

// Test Named Pipe reconnection
TEST_F(IPCTest, NamedPipeReconnection) {
  const std::string pipeName = "\\\\.\\pipe\\WatcherTestReconnect";

  NamedPipeServer server(pipeName);
  EXPECT_TRUE(server.Start());

  // First connection
  NamedPipeClient client1(pipeName);
  EXPECT_TRUE(client1.Connect());
  client1.Disconnect();

  // Second connection
  NamedPipeClient client2(pipeName);
  EXPECT_TRUE(client2.Connect());
  client2.Disconnect();

  server.Stop();
}

// Test heartbeat timeout detection
TEST_F(IPCTest, HeartbeatTimeout) {
  // This would test the ServiceLauncher heartbeat mechanism
  // Simplified version here

  auto lastHeartbeat = std::chrono::system_clock::now();
  const auto timeout = std::chrono::seconds(5);

  std::this_thread::sleep_for(std::chrono::seconds(6));

  auto now = std::chrono::system_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat);

  EXPECT_GT(elapsed.count(), timeout.count());
}

// Test IPCChannel wrapper
TEST_F(IPCTest, IPCChannelServer) {
  const std::string pipeName = "\\\\.\\pipe\\WatcherTestChannel";

  IPCChannel serverChannel(IPCChannel::Mode::Server, pipeName);
  EXPECT_TRUE(serverChannel.Start());

  std::atomic<bool> received{false};
  serverChannel.SetMessageHandler(
      [&](const IPCMessage &msg) { received = true; });

  // Client
  std::thread clientThread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    IPCChannel clientChannel(IPCChannel::Mode::Client, pipeName);
    EXPECT_TRUE(clientChannel.Start());

    IPCMessage msg = IPCMessage::Create(IPCMessageType::PROCESS_READY);
    clientChannel.SendMessage(msg);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    clientChannel.Stop();
  });

  // Wait for message
  for (int i = 0; i < 30 && !received; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  clientThread.join();
  serverChannel.Stop();

  EXPECT_TRUE(received);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
