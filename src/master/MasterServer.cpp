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
    timer->start(5000);
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
    // Buffered reading: simpler for now, assume JSON object per packet or line
    // In prod, need ring buffer. For now, readAll and split by newline if we
    // enforce it, or try parse.
    QByteArray data = socket->readAll();
    // TODO: Handle fragmentation. Assuming small packets for control, large for
    // screenshots. If it's a screenshot, it might be fragmented. Ideally we
    // need a length-prefix protocol.

    // Attempt to parse as JSON string
    std::string jsonStr = data.toStdString();

    try {
      protocol::MessageSerializer serializer;
      auto msg = serializer.Deserialize(jsonStr); // Might throw if incomplete

      processMessage(socket, msg);
    } catch (const std::exception &e) {
      // In a real implementation we would buffer 'data' until valid JSON.
      // LOG_ERROR("Failed to parse message: " + std::string(e.what()));
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
  }

  void checkClientHealth() {
    // Check for timeouts
    time_t now = time(nullptr);
    std::lock_guard<std::mutex> lock(clientsMutex_);
    // Logic to remove stale clients could go here
  }

private:
  void processMessage(QTcpSocket *socket, const protocol::Message &msg) {
    if (msg.type == protocol::CommandType::HELLO) {
      // Register client
      ClientInfo info;
      info.id = msg.source;
      info.hostname = msg.payload.value("hostname", "Unknown");
      info.ip_address = socket->peerAddress().toString().toStdString();
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
      // Extract base64 or raw bytes
      // Payload might contain "data" as base64 string
      if (msg.payload.contains("data")) {
        std::string base64 = msg.payload["data"];
        QByteArray bytes =
            QByteArray::fromBase64(QByteArray::fromStdString(base64));
        std::vector<uint8_t> vec(bytes.begin(), bytes.end());

        notifyClientThumbnailUpdated(msg.source, vec);
        notifyScreenshotReceived(msg.source, vec);
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

    it->second->write(data.data(), data.size());
    it->second->flush();
    return true;
  }

  bool broadcastCommand(protocol::CommandType type) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    protocol::Message msg =
        protocol::Message::Create(type, "MASTER", "BROADCAST");
    protocol::MessageSerializer serializer;
    std::string data = serializer.Serialize(msg);

    for (auto &pair : clientSockets_) {
      pair.second->write(data.data(), data.size());
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

    it->second->write(data.data(), data.size());
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
};

std::unique_ptr<MasterServer> createMasterServer(const ServerConfig &config) {
  // We cannot use make_unique directly because we need to manage lifecycle,
  // but unique_ptr is expected.
  return std::make_unique<MasterServerImpl>(config);
}

} // namespace master
} // namespace cms

#include "MasterServer.moc"
