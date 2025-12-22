#include "cms/IPCChannel.h"
#include "cms/Logger.h"
#include <stdexcept>

#ifdef _WIN32

namespace cms {
namespace ipc {

// ============================================================================
// NamedPipeServer Implementation
// ============================================================================

NamedPipeServer::NamedPipeServer(const std::string &pipeName)
    : pipeName_(pipeName), pipeHandle_(INVALID_HANDLE_VALUE) {}

NamedPipeServer::~NamedPipeServer() { Stop(); }

bool NamedPipeServer::Start() {
  if (running_) {
    LOG_WARNING("NamedPipeServer already running");
    return true;
  }

  LOG_INFO("Starting NamedPipeServer: " + pipeName_);

  running_ = true;

  listenThread_ =
      std::make_unique<std::thread>(&NamedPipeServer::ListenLoop, this);

  return true;
}

void NamedPipeServer::Stop() {
  if (!running_) {
    return;
  }

  LOG_INFO("Stopping NamedPipeServer");

  running_ = false;

  // Close pipe handle to interrupt blocking operations
  if (pipeHandle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(pipeHandle_);
    pipeHandle_ = INVALID_HANDLE_VALUE;
  }

  if (listenThread_ && listenThread_->joinable()) {
    listenThread_->join();
  }

  if (readThread_ && readThread_->joinable()) {
    readThread_->join();
  }
}

bool NamedPipeServer::SendMessage(const IPCMessage &message) {
  try {
    std::string json = serializer_.Serialize(message);

    // Add message length prefix (4 bytes)
    uint32_t length = static_cast<uint32_t>(json.size());

    std::lock_guard<std::mutex> lock(sendMutex_);

    // Write length
    if (!WriteData(&length, sizeof(length))) {
      LOG_ERROR("Failed to write message length");
      return false;
    }

    // Write data
    if (!WriteData(json.data(), json.size())) {
      LOG_ERROR("Failed to write message data");
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    LOG_ERROR("Failed to send IPC message: " + std::string(e.what()));
    return false;
  }
}

void NamedPipeServer::SetMessageHandler(MessageHandler handler) {
  messageHandler_ = handler;
}

HANDLE NamedPipeServer::CreateNamedPipe() {
  return ::CreateNamedPipeA(pipeName_.c_str(),
                            PIPE_ACCESS_DUPLEX,      // Read/write access
                            PIPE_TYPE_BYTE |         // Byte-type pipe
                                PIPE_READMODE_BYTE | // Byte-read mode
                                PIPE_WAIT,           // Blocking mode
                            1, // Max instances (1 for simplicity)
                            IPCConstants::BUFFER_SIZE, // Output buffer size
                            IPCConstants::BUFFER_SIZE, // Input buffer size
                            0,                         // Default timeout
                            NULL                       // Default security
  );
}

void NamedPipeServer::ListenLoop() {
  while (running_) {
    LOG_INFO("Creating named pipe: " + pipeName_);

    pipeHandle_ = CreateNamedPipe();
    if (pipeHandle_ == INVALID_HANDLE_VALUE) {
      LOG_ERROR("CreateNamedPipe failed: " + std::to_string(GetLastError()));
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }

    LOG_INFO("Waiting for client connection...");

    // Wait for client to connect
    BOOL connected = ConnectNamedPipe(pipeHandle_, NULL);
    if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
      LOG_ERROR("ConnectNamedPipe failed: " + std::to_string(GetLastError()));
      CloseHandle(pipeHandle_);
      pipeHandle_ = INVALID_HANDLE_VALUE;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }

    LOG_INFO("Client connected!");

    // Start read thread
    readThread_ =
        std::make_unique<std::thread>(&NamedPipeServer::ReadLoop, this);

    // Wait for read thread to finish (client disconnected)
    if (readThread_->joinable()) {
      readThread_->join();
    }

    // Disconnect and close pipe
    DisconnectNamedPipe(pipeHandle_);
    CloseHandle(pipeHandle_);
    pipeHandle_ = INVALID_HANDLE_VALUE;

    LOG_INFO("Client disconnected");
  }
}

void NamedPipeServer::ReadLoop() {
  std::vector<uint8_t> buffer(IPCConstants::BUFFER_SIZE);

  while (running_) {
    // Read message length (4 bytes)
    uint32_t length = 0;
    size_t bytesRead = 0;

    if (!ReadData(&length, sizeof(length), bytesRead)) {
      LOG_WARNING("Failed to read message length");
      break;
    }

    if (bytesRead != sizeof(length)) {
      LOG_WARNING("Incomplete length read");
      break;
    }

    if (length == 0 || length > IPCConstants::BUFFER_SIZE) {
      LOG_ERROR("Invalid message length: " + std::to_string(length));
      break;
    }

    // Read message data
    buffer.resize(length);
    if (!ReadData(buffer.data(), length, bytesRead)) {
      LOG_WARNING("Failed to read message data");
      break;
    }

    if (bytesRead != length) {
      LOG_WARNING("Incomplete message read");
      break;
    }

    // Deserialize and handle message
    try {
      std::string json(buffer.begin(), buffer.begin() + length);
      IPCMessage msg = serializer_.Deserialize(json);

      if (messageHandler_) {
        messageHandler_(msg);
      }

    } catch (const std::exception &e) {
      LOG_ERROR("Failed to deserialize message: " + std::string(e.what()));
    }
  }
}

bool NamedPipeServer::WriteData(const void *data, size_t size) {
  if (pipeHandle_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD bytesWritten = 0;
  BOOL success = WriteFile(pipeHandle_, data, static_cast<DWORD>(size),
                           &bytesWritten, NULL);

  return success && bytesWritten == size;
}

bool NamedPipeServer::ReadData(void *buffer, size_t size, size_t &bytesRead) {
  if (pipeHandle_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD dwBytesRead = 0;
  BOOL success = ReadFile(pipeHandle_, buffer, static_cast<DWORD>(size),
                          &dwBytesRead, NULL);

  bytesRead = dwBytesRead;
  return success;
}

// ============================================================================
// NamedPipeClient Implementation
// ============================================================================

NamedPipeClient::NamedPipeClient(const std::string &pipeName)
    : pipeName_(pipeName), pipeHandle_(INVALID_HANDLE_VALUE) {}

NamedPipeClient::~NamedPipeClient() { Disconnect(); }

bool NamedPipeClient::Connect() {
  if (connected_) {
    LOG_WARNING("NamedPipeClient already connected");
    return true;
  }

  LOG_INFO("Connecting to named pipe: " + pipeName_);

  // Wait for pipe to be available
  if (!WaitNamedPipeA(pipeName_.c_str(), IPCConstants::CONNECT_TIMEOUT_MS)) {
    LOG_ERROR("WaitNamedPipe failed: " + std::to_string(GetLastError()));
    return false;
  }

  // Open pipe
  pipeHandle_ = CreateFileA(pipeName_.c_str(), GENERIC_READ | GENERIC_WRITE,
                            0,             // No sharing
                            NULL,          // Default security
                            OPEN_EXISTING, // Existing pipe
                            0,             // Default attributes
                            NULL           // No template
  );

  if (pipeHandle_ == INVALID_HANDLE_VALUE) {
    LOG_ERROR("CreateFile failed: " + std::to_string(GetLastError()));
    return false;
  }

  // Set pipe mode
  DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
  if (!SetNamedPipeHandleState(pipeHandle_, &mode, NULL, NULL)) {
    LOG_ERROR("SetNamedPipeHandleState failed: " +
              std::to_string(GetLastError()));
    CloseHandle(pipeHandle_);
    pipeHandle_ = INVALID_HANDLE_VALUE;
    return false;
  }

  connected_ = true;
  LOG_INFO("Connected to named pipe");

  return true;
}

void NamedPipeClient::Disconnect() {
  if (!connected_) {
    return;
  }

  LOG_INFO("Disconnecting from named pipe");

  StopReading();

  connected_ = false;

  if (pipeHandle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(pipeHandle_);
    pipeHandle_ = INVALID_HANDLE_VALUE;
  }
}

bool NamedPipeClient::SendMessage(const IPCMessage &message) {
  if (!connected_) {
    LOG_ERROR("Cannot send message: not connected");
    return false;
  }

  try {
    std::string json = serializer_.Serialize(message);

    // Add message length prefix
    uint32_t length = static_cast<uint32_t>(json.size());

    std::lock_guard<std::mutex> lock(mutex_);

    // Write length
    if (!WriteData(&length, sizeof(length))) {
      LOG_ERROR("Failed to write message length");
      return false;
    }

    // Write data
    if (!WriteData(json.data(), json.size())) {
      LOG_ERROR("Failed to write message data");
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    LOG_ERROR("Failed to send IPC message: " + std::string(e.what()));
    return false;
  }
}

void NamedPipeClient::SetMessageHandler(MessageHandler handler) {
  messageHandler_ = handler;
}

bool NamedPipeClient::StartReading() {
  if (reading_) {
    LOG_WARNING("Already reading");
    return true;
  }

  if (!connected_) {
    LOG_ERROR("Cannot start reading: not connected");
    return false;
  }

  reading_ = true;
  readThread_ = std::make_unique<std::thread>(&NamedPipeClient::ReadLoop, this);

  return true;
}

void NamedPipeClient::StopReading() {
  if (!reading_) {
    return;
  }

  reading_ = false;

  if (readThread_ && readThread_->joinable()) {
    readThread_->join();
  }
}

void NamedPipeClient::ReadLoop() {
  std::vector<uint8_t> buffer(IPCConstants::BUFFER_SIZE);

  while (reading_ && connected_) {
    // Read message length
    uint32_t length = 0;
    size_t bytesRead = 0;

    if (!ReadData(&length, sizeof(length), bytesRead)) {
      LOG_WARNING("Failed to read message length");
      break;
    }

    if (bytesRead != sizeof(length)) {
      LOG_WARNING("Incomplete length read");
      break;
    }

    if (length == 0 || length > IPCConstants::BUFFER_SIZE) {
      LOG_ERROR("Invalid message length: " + std::to_string(length));
      break;
    }

    // Read message data
    buffer.resize(length);
    if (!ReadData(buffer.data(), length, bytesRead)) {
      LOG_WARNING("Failed to read message data");
      break;
    }

    if (bytesRead != length) {
      LOG_WARNING("Incomplete message read");
      break;
    }

    // Deserialize and handle
    try {
      std::string json(buffer.begin(), buffer.begin() + length);
      IPCMessage msg = serializer_.Deserialize(json);

      if (messageHandler_) {
        messageHandler_(msg);
      }

    } catch (const std::exception &e) {
      LOG_ERROR("Failed to deserialize message: " + std::string(e.what()));
    }
  }
}

bool NamedPipeClient::WriteData(const void *data, size_t size) {
  if (pipeHandle_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD bytesWritten = 0;
  BOOL success = WriteFile(pipeHandle_, data, static_cast<DWORD>(size),
                           &bytesWritten, NULL);

  return success && bytesWritten == size;
}

bool NamedPipeClient::ReadData(void *buffer, size_t size, size_t &bytesRead) {
  if (pipeHandle_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD dwBytesRead = 0;
  BOOL success = ReadFile(pipeHandle_, buffer, static_cast<DWORD>(size),
                          &dwBytesRead, NULL);

  bytesRead = dwBytesRead;
  return success;
}

// ============================================================================
// IPCChannel Implementation (High-level wrapper)
// ============================================================================

IPCChannel::IPCChannel(Mode mode, const std::string &pipeName)
    : mode_(mode), pipeName_(pipeName) {}

IPCChannel::~IPCChannel() { Stop(); }

bool IPCChannel::Start() {
  if (mode_ == Mode::Server) {
    server_ = std::make_unique<NamedPipeServer>(pipeName_);
    return server_->Start();
  } else {
    client_ = std::make_unique<NamedPipeClient>(pipeName_);
    if (!client_->Connect()) {
      return false;
    }
    return client_->StartReading();
  }
}

void IPCChannel::Stop() {
  if (server_) {
    server_->Stop();
    server_.reset();
  }
  if (client_) {
    client_->Disconnect();
    client_.reset();
  }
}

bool IPCChannel::SendMessage(const IPCMessage &message) {
  if (server_) {
    return server_->SendMessage(message);
  } else if (client_) {
    return client_->SendMessage(message);
  }
  return false;
}

bool IPCChannel::SendRequest(const IPCMessage &request, IPCMessage &response,
                             uint32_t timeoutMs) {
  // Send request
  if (!SendMessage(request)) {
    return false;
  }

  // Wait for response with matching message ID
  std::unique_lock<std::mutex> lock(responseMutex_);

  bool received =
      responseCv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&]() {
        return pendingResponses_.find(request.message_id) !=
               pendingResponses_.end();
      });

  if (!received) {
    return false;
  }

  response = pendingResponses_[request.message_id];
  pendingResponses_.erase(request.message_id);

  return true;
}

void IPCChannel::SetMessageHandler(MessageHandler handler) {
  // Wrap handler to track responses
  auto wrappedHandler = [this, handler](const IPCMessage &msg) {
    // Check if this is a response to a pending request
    std::lock_guard<std::mutex> lock(responseMutex_);
    if (pendingResponses_.find(msg.message_id) == pendingResponses_.end()) {
      // Not a response, call user handler
      if (handler) {
        handler(msg);
      }
    } else {
      // This is a response
      pendingResponses_[msg.message_id] = msg;
      responseCv_.notify_all();
    }
  };

  if (server_) {
    server_->SetMessageHandler(wrappedHandler);
  } else if (client_) {
    client_->SetMessageHandler(wrappedHandler);
  }
}

bool IPCChannel::IsActive() const {
  if (server_) {
    return server_->IsRunning();
  } else if (client_) {
    return client_->IsConnected();
  }
  return false;
}

} // namespace ipc
} // namespace cms

#endif // _WIN32
