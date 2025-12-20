#include "cms/MasterServer.h"
#include "cms/Logger.h"
#include "cms/Protocol.h"
#include <QHostAddress>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <algorithm>
#include <iostream>
#include <map>
#include <mutex>

namespace cms {
namespace master {

class MasterServerImpl : public QObject, public MasterServer {
  Q_OBJECT

public:
  explicit MasterServerImpl(const ServerConfig &config) : config_(config) {
    tcpServer_ = new QTcpServer(this);
    connect(tcpServer_, &QTcpServer::newConnection, this,
            &MasterServerImpl::handleNewConnection);

    // Timer for cleanup/heartbeat checks (every 5 seconds)
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this,
            &MasterServerImpl::checkClientHealth);
    timer->start(5555);
  }

  bool start() override {
    // Run on main thread if not already
    bool success = false;
    // Since we are likely on main thread, just call listen.
    // If called from another thread, we might need invokeMethod, but QTcpServer
    // expects to be used in its thread.
    if (tcpServer_->listen(QHostAddress::Any, config_.port)) {
      LOG_INFO("Master Server listening on port " +
               std::to_string(config_.port));
      success = true;
    } else {
      LOG_ERROR("Failed to start Master Server: " +
                tcpServer_->errorString().toStdString());
    }
    return success;
  }

  bool stop() override {
    tcpServer_->close();
    // Disconnect all clients
    for (auto &pair : clientSockets_) {
      pair.second->disconnectFromHost();
    }
    clientSockets_.clear();
    clients_.clear();
    LOG_INFO("Master Server stopped");
    return true;
  }

  bool isRunning() const override { return tcpServer_->isListening(); }

  std::vector<ClientInfo> getConnectedClients() const override {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    std::vector<ClientInfo> list;
    for (const auto &pair : clients_) {
      list.push_back(pair.second);
    }
    return list;
  }

  ClientInfo getClientInfo(const std::string &client_id) const override {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(client_id);
    if (it != clients_.end()) {
      return it->second;
    }
    return ClientInfo{};
  }

  bool lockClient(const std::string &client_id) override {
    return sendCommand(client_id, protocol::CommandType::SCREEN_LOCK);
  }

  bool unlockClient(const std::string &client_id) override {
    return sendCommand(client_id, protocol::CommandType::SCREEN_UNLOCK);
  }

  bool lockAll() override {
    return broadcastCommand(protocol::CommandType::SCREEN_LOCK);
  }

  bool unlockAll() override {
    return broadcastCommand(protocol::CommandType::SCREEN_UNLOCK);
  }

  bool broadcastMessage(const std::string &message) override {
    // Not implemented in protocol yet as a specific command, maybe
    // STATUS_UPDATE or similar
    return true;
  }

  bool refreshClientThumbnail(const std::string &client_id) override {
    return sendCommand(client_id, protocol::CommandType::SCREENSHOT_REQUEST);
  }

  void addObserver(IServerObserver *observer) override {
    std::lock_guard<std::mutex> lock(observers_mutex_);
    observers_.push_back(observer);
  }

  void removeObserver(IServerObserver *observer) override {
    std::lock_guard<std::mutex> lock(observers_mutex_);
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end());
  }

private slots:
  void handleNewConnection() {
    while (tcpServer_->hasPendingConnections()) {
      QTcpSocket *clientSocket = tcpServer_->nextPendingConnection();
      connect(clientSocket, &QTcpSocket::readyRead, this,
              [this, clientSocket]() { handleReadyRead(clientSocket); });
      connect(
          clientSocket, &QTcpSocket::disconnected, this,
          [this, clientSocket]() { handleClientDisconnected(clientSocket); });

      LOG_INFO("New incoming connection from " +
               clientSocket->peerAddress().toString().toStdString());
    }
  }

  void handleReadyRead(QTcpSocket *socket) {
    QByteArray data = socket->readAll();
    std::string &buffer = incomingBuffers_[socket];
    buffer.append(data.toStdString());

    LOG_DEBUG("Received data: " + std::to_string(data.size()) + " bytes from " +
              socket->peerAddress().toString().toStdString());

    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos) {
      std::string line = buffer.substr(0, pos);
      buffer.erase(0, pos + 1);

      if (line.empty())
        continue;

      LOG_DEBUG("Processing line: " + line.substr(0, 100) + "...");

      try {
        protocol::MessageSerializer serializer;
        auto msg = serializer.Deserialize(line);
        LOG_INFO("Successfully parsed message type: " +
                 protocol::CommandTypeToString(msg.type));
        processMessage(socket, msg);
      } catch (const std::exception &e) {
        LOG_ERROR("Failed to parse message: " + std::string(e.what()) +
                  " | Raw data: " + line.substr(0, 200));
      }
    }
  }

  void handleClientDisconnected(QTcpSocket *socket) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    std::string clientIdToRemove;

    // Find client ID for this socket
    for (auto it = clientSockets_.begin(); it != clientSockets_.end(); ++it) {
      if (it->second == socket) {
        clientIdToRemove = it->first;
        clientSockets_.erase(it);
        break;
      }
    }

    if (!clientIdToRemove.empty()) {
      clients_.erase(clientIdToRemove);
      notifyClientDisconnected(clientIdToRemove);
      LOG_INFO("Client disconnected: " + clientIdToRemove);
    }

    socket->deleteLater();

    // Clean up buffer
    incomingBuffers_.erase(socket);
  }

  void checkClientHealth() {
    // Check for timeouts
    time_t now = time(nullptr);
    std::vector<std::string> staleClients;

    {
      std::lock_guard<std::mutex> lock(clientsMutex_);
      for (const auto &pair : clients_) {
        if (difftime(now, pair.second.last_heartbeat) >
            60) { // 60 seconds timeout
          staleClients.push_back(pair.first);
        }
      }
    }

    for (const auto &clientId : staleClients) {
      std::lock_guard<std::mutex> lock(clientsMutex_);
      auto it = clientSockets_.find(clientId);
      if (it != clientSockets_.end()) {
        LOG_WARNING("Client timed out: " + clientId);
        it->second->disconnectFromHost();
      }
    }
  }

private:
  void processMessage(QTcpSocket *socket, const protocol::Message &msg) {
    if (msg.type == protocol::CommandType::HELLO) {
      // Register client
      ClientInfo info;
      info.id = msg.source;

      // Extract hostname with fallback to machine_id
      info.hostname = msg.payload.value(
          "hostname", msg.payload.value("machine_id", "Unknown"));

      // Normalize IP address (convert IPv6-mapped IPv4 to pure IPv4)
      QHostAddress addr = socket->peerAddress();
      if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        bool ok;
        QHostAddress ipv4 = QHostAddress(addr.toIPv4Address(&ok));
        if (ok) {
          addr = ipv4;
        }
      }
      info.ip_address = addr.toString().toStdString();

      info.state = ClientState::CONNECTED;
      info.last_heartbeat = time(nullptr);

      {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_[info.id] = info;
        clientSockets_[info.id] = socket;
      }

      notifyClientConnected(info);
      LOG_INFO("Registered client: " + info.id);

    } else if (msg.type == protocol::CommandType::SCREENSHOT_DATA) {
      LOG_DEBUG("Processing SCREENSHOT_DATA from: " + msg.source);
      if (msg.payload.contains("data")) {
        std::string base64 = msg.payload["data"];
        QByteArray bytes =
            QByteArray::fromBase64(QByteArray::fromStdString(base64));
        std::vector<uint8_t> vec(bytes.begin(), bytes.end());

        // Dimensions are in the payload but don't need to be stored in
        // ClientInfo

        notifyClientThumbnailUpdated(msg.source, vec);
        notifyScreenshotReceived(msg.source, vec);
        LOG_DEBUG("Screenshot data processed for: " + msg.source);
      }
    } else if (msg.type == protocol::CommandType::THUMBNAIL_UPDATE) {
      // Handle periodic thumbnail updates
      if (msg.payload.contains("data")) {
        std::string base64 = msg.payload["data"];
        QByteArray bytes =
            QByteArray::fromBase64(QByteArray::fromStdString(base64));
        std::vector<uint8_t> vec(bytes.begin(), bytes.end());

        // Dimensions are in the payload and used by the UI

        // Notify UI to update thumbnail
        notifyClientThumbnailUpdated(msg.source, vec);
        LOG_DEBUG("Thumbnail updated: " + msg.source);
      }
    } else if (msg.type == protocol::CommandType::PING) {
      std::lock_guard<std::mutex> lock(clientsMutex_);
      if (clients_.count(msg.source)) {
        clients_[msg.source].last_heartbeat = time(nullptr);
      }
    }
  }

  bool sendCommand(const std::string &client_id, protocol::CommandType type) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clientSockets_.find(client_id);
    if (it == clientSockets_.end())
      return false;

    protocol::Message msg =
        protocol::Message::Create(type, "MASTER", client_id);
    protocol::MessageSerializer serializer;
    std::string data = serializer.Serialize(msg);

    // Add newline delimiter so client can parse the message
    std::string packet = data + "\n";

    LOG_DEBUG("Sending " + protocol::CommandTypeToString(type) + " to " +
              client_id);

    it->second->write(packet.data(), packet.size());
    it->second->flush();
    return true;
  }

  bool broadcastCommand(protocol::CommandType type) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    protocol::Message msg =
        protocol::Message::Create(type, "MASTER", "BROADCAST");
    protocol::MessageSerializer serializer;
    std::string data = serializer.Serialize(msg);

    // Add newline delimiter
    std::string packet = data + "\n";

    for (auto &pair : clientSockets_) {
      pair.second->write(packet.data(), packet.size());
      pair.second->flush();
    }
    return true;
  }

  bool sendDomainPolicy(const std::string &client_id,
                        const DomainPolicy &policy) override {
    nlohmann::json payload = policy.toJson();

    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clientSockets_.find(client_id);
    if (it == clientSockets_.end())
      return false;

    protocol::Message msg =
        protocol::Message::Create(protocol::CommandType::DOMAIN_POLICY_UPDATE,
                                  "MASTER", client_id, payload);
    protocol::MessageSerializer serializer;
    std::string data = serializer.Serialize(msg);

    // Add newline delimiter
    std::string packet = data + "\n";

    it->second->write(packet.data(), packet.size());
    it->second->flush();
    return true;
  }

  void notifyClientConnected(const ClientInfo &info) {
    std::lock_guard<std::mutex> lock(observers_mutex_);
    for (auto *obs : observers_)
      obs->onClientConnected(info);
  }

  void notifyClientDisconnected(const std::string &id) {
    std::lock_guard<std::mutex> lock(observers_mutex_);
    for (auto *obs : observers_)
      obs->onClientDisconnected(id);
  }

  void notifyClientThumbnailUpdated(const std::string &id,
                                    const std::vector<uint8_t> &data) {
    std::lock_guard<std::mutex> lock(observers_mutex_);
    for (auto *obs : observers_)
      obs->onClientThumbnailUpdated(id, data);
  }

  void notifyScreenshotReceived(const std::string &id,
                                const std::vector<uint8_t> &data) {
    std::lock_guard<std::mutex> lock(observers_mutex_);
    for (auto *obs : observers_)
      obs->onScreenshotReceived(id, data);
  }

  ServerConfig config_;
  QTcpServer *tcpServer_;

  mutable std::mutex clientsMutex_;
  std::map<std::string, ClientInfo> clients_;
  std::map<std::string, QTcpSocket *> clientSockets_;

  std::vector<IServerObserver *> observers_;

  std::mutex observers_mutex_;

  // Buffers for incoming data
  std::map<QTcpSocket *, std::string> incomingBuffers_;
};

std::unique_ptr<MasterServer> createMasterServer(const ServerConfig &config) {
  // We cannot use make_unique directly because we need to manage lifecycle,
  // but unique_ptr is expected.
  return std::make_unique<MasterServerImpl>(config);
}

} // namespace master
} // namespace cms

#include "MasterServer.moc"
