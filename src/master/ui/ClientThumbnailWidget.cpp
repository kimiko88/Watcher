#include "cms/ui/ClientThumbnailWidget.h"
#include <QPainter>
#include <QMouseEvent>

namespace cms {
namespace ui {

ClientThumbnailWidget::ClientThumbnailWidget(QWidget* parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    image_label_ = new QLabel("No Image", this);
    image_label_->setAlignment(Qt::AlignCenter);
    image_label_->setStyleSheet("background-color: #333; color: #aaa; border: 1px solid #555;");
    image_label_->setMinimumSize(160, 90);
    
    name_label_ = new QLabel("Client", this);
    name_label_->setAlignment(Qt::AlignCenter);
    
    status_label_ = new QLabel("Disconnected", this);
    status_label_->setAlignment(Qt::AlignCenter);
    status_label_->setStyleSheet("color: red; font-size: 10px;");
    
    layout->addWidget(image_label_);
    layout->addWidget(name_label_);
    layout->addWidget(status_label_);
    layout->setContentsMargins(5, 5, 5, 5);
    
    setCursor(Qt::PointingHandCursor);
}

void ClientThumbnailWidget::setClientInfo(const master::ClientInfo& info) {
    client_id_ = info.id;
    name_label_->setText(QString::fromStdString(info.hostname));
    updateStyle(info.state);
}

void ClientThumbnailWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(client_id_);
    }
    QWidget::mousePressEvent(event);
}

void ClientThumbnailWidget::updateStyle(master::ClientState state) {
    switch (state) {
        case master::ClientState::CONNECTED:
            status_label_->setText("Connected");
            status_label_->setStyleSheet("color: green;");
            break;
        case master::ClientState::LOCKED:
            status_label_->setText("LOCKED");
            status_label_->setStyleSheet("color: orange; font-weight: bold;");
            break;
        case master::ClientState::DISCONNECTED:
            status_label_->setText("Disconnected");
            status_label_->setStyleSheet("color: red;");
            break;
        default: break;
    }
}

} // namespace ui
} // namespace cms
