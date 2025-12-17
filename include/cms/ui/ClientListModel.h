#ifndef CMS_UI_CLIENT_LIST_MODEL_H
#define CMS_UI_CLIENT_LIST_MODEL_H

#include <QAbstractTableModel>
#include <vector>
#include "cms/MasterServer.h"

namespace cms {
namespace ui {

class ClientListModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum ClientRoles {
        IdRole = Qt::UserRole + 1,
        HostnameRole,
        IpRole,
        StateRole,
        ThumbnailRole,
        LastHeartbeatRole
    };

    explicit ClientListModel(QObject* parent = nullptr);
    
    // AbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Data manipulation
    void updateClients(const std::vector<master::ClientInfo>& clients);
    void addClient(const master::ClientInfo& client);
    void removeClient(const std::string& client_id);
    master::ClientInfo getClient(int row) const;

private:
    std::vector<master::ClientInfo> clients_;
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_CLIENT_LIST_MODEL_H
