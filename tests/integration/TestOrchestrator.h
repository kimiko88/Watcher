#ifndef TEST_ORCHESTRATOR_H
#define TEST_ORCHESTRATOR_H

#include "cms/ClientService.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "cms/Socket.h"

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

  void sendCommand(const std::string &clientId, const std::string &cmd) {
    // Mock send
    std::lock_guard<std::mutex> lock(clients_mutex_);
    if (!clients_.empty()) {
      clients_[0]->Send(cmd + "\n");
    }
  }

  // Check if we received a specific message type (stub)
  bool hasReceivedMessage(const std::string &type) {
    return false; // TODO: Implement read loop
  }

private:
  int port_;
  std::atomic<bool> running_;
  std::thread server_thread_;
  std::unique_ptr<cms::Socket> server_socket_;
  std::vector<std::unique_ptr<cms::Socket>> clients_;
  std::mutex clients_mutex_;
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
