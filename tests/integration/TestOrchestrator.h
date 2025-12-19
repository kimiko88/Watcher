#ifndef TEST_ORCHESTRATOR_H
#define TEST_ORCHESTRATOR_H

#include "cms/ClientService.h"
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// Simple Mock Master Server for testing Client Service
class MockMasterServer {
public:
  MockMasterServer(int port)
      : port_(port), running_(false), server_socket_(nullptr) {}

  void start() {
    if (running_)
      return;
    running_ = true;
    server_thread_ = std::thread([this]() {
      cms::Socket::Initialize();
      server_socket_ = std::make_unique<cms::Socket>();

      if (!server_socket_->Bind(port_)) {
        std::cerr << "MockMaster: Bind failed on port " << port_ << std::endl;
        return;
      }
      if (!server_socket_->Listen()) {
        std::cerr << "MockMaster: Listen failed" << std::endl;
        return;
      }

      std::cout << "MockMaster listening on " << port_ << std::endl;

      while (running_) {
        if (!server_socket_->IsValid())
          break;

        cms::Socket *client = server_socket_->Accept();
        if (client) {
          std::cout << "MockMaster: Client connected" << std::endl;
          std::lock_guard<std::mutex> lock(clients_mutex_);
          clients_.push_back(std::unique_ptr<cms::Socket>(client));

          // Handshake wait (simplified for now)
          // Start reading from client
          client_threads_.emplace_back(&MockMasterServer::readLoop, this,
                                       client);
        } else {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
    });
  }

  void stop() {
    running_ = false;
    if (server_socket_)
      server_socket_->Close();
    if (server_thread_.joinable())
      server_thread_.join();
    cms::Socket::Cleanup();

    std::lock_guard<std::mutex> lock(clients_mutex_);
    // Close all client sockets to unblock read loops
    for (auto &c : clients_) {
      if (c)
        c->Close();
    }

    for (auto &t : client_threads_) {
      if (t.joinable())
        t.join();
    }
    client_threads_.clear();
    clients_.clear();
  }

  // Mock Helpers
  bool waitForClient(const std::string &clientId, int timeoutMs = 5000) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start <
           std::chrono::milliseconds(timeoutMs)) {
      std::lock_guard<std::mutex> lock(clients_mutex_);
      if (!clients_.empty())
        return true; // simplified connect check
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
  }

  void sendCommand(const std::string &clientId, const std::string &cmdType,
                   const nlohmann::json &payload = {}) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    if (!clients_.empty()) {
      // Create message
      auto msg = cms::protocol::Message::Create(
          cms::protocol::StringToCommandType(cmdType), "master", clientId,
          payload);
      cms::protocol::MessageSerializer serializer;
      std::string json = serializer.Serialize(msg);
      clients_[0]->Send(json + "\n");
    }
  }

  bool waitForMessage(const std::string &type, int timeoutMs = 5000) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start <
           std::chrono::milliseconds(timeoutMs)) {
      std::lock_guard<std::mutex> lock(messages_mutex_);
      for (const auto &msg : received_messages_) {
        if (cms::protocol::CommandTypeToString(msg.type) == type) {
          return true;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
  }

  cms::protocol::Message getLastMessage() {
    std::lock_guard<std::mutex> lock(messages_mutex_);
    if (received_messages_.empty()) {
      return {};
    }
    return received_messages_.back();
  }

private:
  void readLoop(cms::Socket *client) {
    cms::protocol::MessageSerializer serializer;
    std::string buffer;
    char tempBuffer[4096];

    while (running_ && client->IsValid()) {
      int bytesRead = client->Receive(tempBuffer, sizeof(tempBuffer) - 1);
      if (bytesRead > 0) {
        tempBuffer[bytesRead] = '\0';
        buffer += tempBuffer;

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
          std::string line = buffer.substr(0, pos);
          buffer.erase(0, pos + 1);

          try {
            auto msg = serializer.Deserialize(line);
            std::lock_guard<std::mutex> lock(messages_mutex_);
            received_messages_.push_back(msg);
            std::cout << "MockMaster: Received "
                      << cms::protocol::CommandTypeToString(msg.type)
                      << std::endl;
          } catch (const std::exception &e) {
            std::cerr << "MockMaster: Failed to parse message: " << e.what()
                      << std::endl;
          }
        }
      } else {
        break;
      }
    }
  }

  int port_;
  std::atomic<bool> running_;
  std::thread server_thread_;
  std::unique_ptr<cms::Socket> server_socket_;
  std::vector<std::unique_ptr<cms::Socket>> clients_;
  std::mutex clients_mutex_;

  std::vector<cms::protocol::Message> received_messages_;
  std::mutex messages_mutex_;
  std::vector<std::thread> client_threads_;
};

// Orchestrator to manage multiple components
class TestOrchestrator {
public:
  TestOrchestrator() = default;

  void startMockMaster(int port = 5555) {
    master_ = std::make_unique<MockMasterServer>(port);
    master_->start();
  }

  void stopMockMaster() {
    if (master_)
      master_->stop();
  }

  // Helper to run a real client service in a thread
  void startRealClient(const std::string &configPath) {
    auto client = std::make_shared<cms::client::ClientService>(configPath);
    clients_.push_back(client);

    threads_.emplace_back([client]() { client->start(); });
  }

  MockMasterServer *getMaster() { return master_.get(); }

  void cleanup() {
    stopMockMaster();
    for (auto &c : clients_) {
      c->stop();
    }
    for (auto &t : threads_) {
      if (t.joinable())
        t.join();
    }
    clients_.clear();
    threads_.clear();
  }

  ~TestOrchestrator() { cleanup(); }

private:
  std::unique_ptr<MockMasterServer> master_;
  std::vector<std::shared_ptr<cms::client::ClientService>> clients_;
  std::vector<std::thread> threads_;
};

#endif // TEST_ORCHESTRATOR_H
