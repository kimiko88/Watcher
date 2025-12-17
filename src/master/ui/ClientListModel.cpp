#include "cms/ui/ClientListModel.h"

namespace cms {
namespace ui {

ClientListModel::ClientListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int ClientListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(clients_.size());
}

int ClientListModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return 4; // ID, Hostname, IP, State
}

QVariant ClientListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= clients_.size()) return QVariant();

    const auto& client = clients_[index.row()];

    switch (role) {
        case IdRole: return QString::fromStdString(client.id);
        case HostnameRole: return QString::fromStdString(client.hostname);
        case IpRole: return QString::fromStdString(client.ip_address);
        case StateRole: return static_cast<int>(client.state);
        case ThumbnailRole: return QByteArray(); // TODO: Convert vector<uint8_t> to QImage/QPixmap?
        
        case Qt::DisplayRole:
            switch(index.column()) {
                case 0: return QString::fromStdString(client.hostname);
                case 1: return QString::fromStdString(client.ip_address);
                case 2: return client.isConnected() ? "Connected" : "Disconnected";
                default: return "";
            }
        default: return QVariant();
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

void ClientListModel::updateClients(const std::vector<master::ClientInfo>& clients) {
    beginResetModel();
    clients_ = clients;
    endResetModel();
}

void ClientListModel::addClient(const master::ClientInfo& client) {
    beginInsertRows(QModelIndex(), clients_.size(), clients_.size());
    clients_.push_back(client);
    endInsertRows();
}

void ClientListModel::removeClient(const std::string& client_id) {
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

} // namespace ui
} // namespace cms
