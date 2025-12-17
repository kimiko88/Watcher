#ifndef CMS_UI_CLIENT_THUMBNAIL_WIDGET_H
#define CMS_UI_CLIENT_THUMBNAIL_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "cms/MasterServer.h"

namespace cms {
namespace ui {

class ClientThumbnailWidget : public QWidget {
    Q_OBJECT

public:
    explicit ClientThumbnailWidget(QWidget* parent = nullptr);
    
    void setClientInfo(const master::ClientInfo& info);
    std::string getClientId() const { return client_id_; }

signals:
    void clicked(const std::string& client_id);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    std::string client_id_;
    QLabel* image_label_;
    QLabel* name_label_;
    QLabel* status_label_;
    
    void updateStyle(master::ClientState state);
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_CLIENT_THUMBNAIL_WIDGET_H
