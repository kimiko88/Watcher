#ifndef CMS_IPC_CHANNEL_H
#define CMS_IPC_CHANNEL_H

#include "Common.h"
#include "IPCProtocol.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace cms {
namespace ipc {

// Message handler callback
using MessageHandler = std::function<void(const IPCMessage &)>;

// ============================================================================
// NAMED PIPE SERVER
// ============================================================================

// Named Pipe Server (used by service to accept connections)
class NamedPipeServer {
public:
  // Constructor
  // pipeName: Named pipe path (e.g., "\\\\.\\pipe\\MyPipe")
  explicit NamedPipeServer(const std::string &pipeName);

  // Destructor
  ~NamedPipeServer();

  // Start the server (create pipe and listen for connections)
  bool Start();

  // Stop the server
  void Stop();

  // Check if server is running
  bool IsRunning() const { return running_; }

  // Send message to connected client
  bool SendIPCMessage(const IPCMessage &message);

  // Set message handler callback
  void SetMessageHandler(MessageHandler handler);

private:
  std::string pipeName_;
  HANDLE pipeHandle_;
  std::atomic<bool> running_{false};
  std::unique_ptr<std::thread> listenThread_;
  std::unique_ptr<std::thread> readThread_;
  MessageHandler messageHandler_;
  IPCMessageSerializer serializer_;

  mutable std::mutex mutex_;
  std::queue<std::string> sendQueue_;
  std::mutex sendMutex_;

  // Listen for incoming connections
  void ListenLoop();

  // Read messages from client
  void ReadLoop();

  // Create named pipe
  HANDLE CreateNamedPipe();

  // Write data to pipe
  bool WriteData(const void *data, size_t size);

  // Read data from pipe
  bool ReadData(void *buffer, size_t size, size_t &bytesRead);
};

// ============================================================================
// NAMED PIPE CLIENT
// ============================================================================

// Named Pipe Client (used by worker/GUI to connect to service)
class NamedPipeClient {
public:
  // Constructor
  // pipeName: Named pipe path to connect to
  explicit NamedPipeClient(const std::string &pipeName);

  // Destructor
  ~NamedPipeClient();

  // Connect to the pipe server
  bool Connect();

  // Disconnect from server
  void Disconnect();

  // Check if connected
  bool IsConnected() const { return connected_; }

  // Send message to server
  bool SendIPCMessage(const IPCMessage &message);

  // Set message handler callback
  void SetMessageHandler(MessageHandler handler);

  // Start reading messages (in separate thread)
  bool StartReading();

  // Stop reading messages
  void StopReading();

private:
  std::string pipeName_;
  HANDLE pipeHandle_;
  std::atomic<bool> connected_{false};
  std::atomic<bool> reading_{false};
  std::unique_ptr<std::thread> readThread_;
  MessageHandler messageHandler_;
  IPCMessageSerializer serializer_;

  mutable std::mutex mutex_;

  // Read loop (runs in separate thread)
  void ReadLoop();

  // Write data to pipe
  bool WriteData(const void *data, size_t size);

  // Read data from pipe
  bool ReadData(void *buffer, size_t size, size_t &bytesRead);
};

// ============================================================================
// IPC CHANNEL (High-level wrapper)
// ============================================================================

// High-level IPC Channel wrapper
// Provides request-response pattern on top of Named Pipes
class IPCChannel {
public:
  // Create as server or client
  enum class Mode { Server, Client };

  // Constructor
  IPCChannel(Mode mode, const std::string &pipeName);

  // Destructor
  ~IPCChannel();

  // Start the channel
  bool Start();

  // Stop the channel
  void Stop();

  // Send message
  bool SendIPCMessage(const IPCMessage &message);

  // Send message and wait for response
  bool SendRequest(const IPCMessage &request, IPCMessage &response,
                   uint32_t timeoutMs = 5000);

  // Set message handler
  void SetMessageHandler(MessageHandler handler);

  // Check if channel is active
  bool IsActive() const;

private:
  Mode mode_;
  std::string pipeName_;
  std::unique_ptr<NamedPipeServer> server_;
  std::unique_ptr<NamedPipeClient> client_;

  // Response tracking for request-response pattern
  std::mutex responseMutex_;
  std::condition_variable responseCv_;
  std::map<std::string, IPCMessage> pendingResponses_;
};

} // namespace ipc
} // namespace cms

#endif // CMS_IPC_CHANNEL_H
