#include "cms/ui/ClientListModel.h"
#include <QImage>

namespace cms {
namespace ui {

ClientListModel::ClientListModel(QObject *parent)
    : QAbstractTableModel(parent) {}

int ClientListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(clients_.size());
}

int ClientListModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return 4; // ID, Hostname, IP, State
}

QVariant ClientListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= clients_.size())
    return QVariant();

  const auto &client = clients_[index.row()];

  switch (role) {
  case IdRole:
    return QString::fromStdString(client.id);
  case HostnameRole:
    return QString::fromStdString(client.hostname);
  case IpRole:
    return QString::fromStdString(client.ip_address);
  case StateRole:
    return static_cast<int>(client.state);
  case ThumbnailRole:
    if (!client.thumbnail_data.empty()) {
      if (client.thumbnail_width > 0 && client.thumbnail_height > 0) {
        // Assume RGBA for now if dimensions are provided
        QImage img(client.thumbnail_data.data(), client.thumbnail_width,
                   client.thumbnail_height, QImage::Format_RGBA8888);
        return img.copy(); // Return a copy to own the data
      } else {
        // Try loading from data (e.g. PNG/JPEG)
        QImage img;
        if (img.loadFromData(client.thumbnail_data.data(),
                             static_cast<int>(client.thumbnail_data.size()))) {
          return img;
        }
      }
    }
    return QVariant();

  case Qt::DisplayRole:
    switch (index.column()) {
    case 0:
      return QString::fromStdString(client.hostname);
    case 1:
      return QString::fromStdString(client.ip_address);
    case 2:
      return client.isConnected() ? "Connected" : "Disconnected";
    default:
      return "";
    }
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> ClientListModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[IdRole] = "clientId";
  roles[HostnameRole] = "hostname";
  roles[IpRole] = "ipAddress";
  roles[StateRole] = "state";
  roles[ThumbnailRole] = "thumbnail";
  return roles;
}

void ClientListModel::updateClients(
    const std::vector<master::ClientInfo> &clients) {
  beginResetModel();
  clients_ = clients;
  endResetModel();
}

void ClientListModel::addClient(const master::ClientInfo &client) {
  // Check if client already exists
  for (int i = 0; i < clients_.size(); ++i) {
    if (clients_[i].id == client.id) {
      // Update existing client
      clients_[i] = client;
      emit dataChanged(index(i, 0), index(i, 3),
                       {Qt::DisplayRole, StateRole, IpRole, HostnameRole});
      return;
    }
  }

  // Add new client
  int row = static_cast<int>(clients_.size());
  beginInsertRows(QModelIndex(), row, row);
  clients_.push_back(client);
  endInsertRows();
}

void ClientListModel::removeClient(const std::string &client_id) {
  for (int i = 0; i < clients_.size(); ++i) {
    if (clients_[i].id == client_id) {
      beginRemoveRows(QModelIndex(), i, i);
      clients_.erase(clients_.begin() + i);
      endRemoveRows();
      return;
    }
  }
}

master::ClientInfo ClientListModel::getClient(int row) const {
  if (row >= 0 && row < clients_.size()) {
    return clients_[row];
  }
  return master::ClientInfo{};
}

void ClientListModel::onClientConnected(const master::ClientInfo &client) {
  // Ensure UI update happens on main thread
  QMetaObject::invokeMethod(
      this, [this, client]() { addClient(client); }, Qt::QueuedConnection);
}

void ClientListModel::onClientDisconnected(const std::string &client_id) {
  QMetaObject::invokeMethod(
      this, [this, client_id]() { removeClient(client_id); },
      Qt::QueuedConnection);
}

void ClientListModel::onClientStateChanged(const std::string &client_id,
                                           master::ClientState new_state) {
  QMetaObject::invokeMethod(
      this,
      [this, client_id, new_state]() {
        for (int i = 0; i < clients_.size(); ++i) {
          if (clients_[i].id == client_id) {
            clients_[i].state = new_state;
            emit dataChanged(index(i, 0), index(i, 3),
                             {StateRole, Qt::DisplayRole});
            break;
          }
        }
      },
      Qt::QueuedConnection);
}

void ClientListModel::onClientThumbnailUpdated(const std::string &client_id,
                                               const std::vector<uint8_t> &data,
                                               int width, int height) {
  QMetaObject::invokeMethod(
      this,
      [this, client_id, data, width, height]() {
        for (int i = 0; i < clients_.size(); ++i) {
          if (clients_[i].id == client_id) {
            clients_[i].thumbnail_data = data;
            clients_[i].thumbnail_width = width;
            clients_[i].thumbnail_height = height;
            emit dataChanged(index(i, 0), index(i, 0), {ThumbnailRole});
            break;
          }
        }
      },
      Qt::QueuedConnection);
}

} // namespace ui
} // namespace cms
