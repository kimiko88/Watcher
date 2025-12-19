#include "cms/ui/ClientThumbnailWidget.h"
#include <QContextMenuEvent>
#include <QImage>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>

namespace cms {
namespace ui {

ClientThumbnailWidget::ClientThumbnailWidget(QWidget *parent)
    : QWidget(parent) {
  auto layout = new QVBoxLayout(this);
  image_label_ = new QLabel("No Image", this);
  image_label_->setAlignment(Qt::AlignCenter);
  image_label_->setStyleSheet(
      "background-color: #333; color: #aaa; border: 1px solid #555;");
  image_label_->setMinimumSize(160, 90);

  name_label_ = new QLabel("Client", this);
  name_label_->setAlignment(Qt::AlignCenter);
  name_label_->setStyleSheet("font-weight: bold;");

  status_label_ = new QLabel("Disconnected", this);
  status_label_->setAlignment(Qt::AlignCenter);
  status_label_->setStyleSheet("color: red; font-size: 10px;");

  layout->addWidget(image_label_);
  layout->addWidget(name_label_);
  layout->addWidget(status_label_);
  layout->setContentsMargins(5, 5, 5, 5);

  setCursor(Qt::PointingHandCursor);
}

void ClientThumbnailWidget::setClientInfo(const master::ClientInfo &info) {
  client_id_ = info.id;
  name_label_->setText(QString::fromStdString(info.hostname) + "\n" +
                       QString::fromStdString(info.ip_address));
  updateStyle(info.state);

  if (!info.thumbnail_data.empty()) {
    QImage img;
    bool loaded = false;
    if (info.thumbnail_width > 0 && info.thumbnail_height > 0) {
      img = QImage(info.thumbnail_data.data(), info.thumbnail_width,
                   info.thumbnail_height, QImage::Format_RGBA8888);
      // Make a deep copy to ensure we own the data if we were to store it,
      // but here we convert to pixmap immediately.
      if (!img.isNull())
        loaded = true;
    }

    if (!loaded) {
      loaded = img.loadFromData(info.thumbnail_data.data(),
                                static_cast<int>(info.thumbnail_data.size()));
    }

    if (loaded) {
      QPixmap pixmap = QPixmap::fromImage(img).scaled(
          image_label_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

      // Draw overlay if locked
      if (info.state == master::ClientState::LOCKED) {
        QPainter painter(&pixmap);
        painter.setBrush(QColor(0, 0, 0, 128));
        painter.drawRect(pixmap.rect());
        painter.setPen(Qt::white);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "LOCKED");
      }

      image_label_->setPixmap(pixmap);
      image_label_->setText("");
    } else {
      image_label_->setText("No Image");
      image_label_->setPixmap(QPixmap());
    }
  } else {
    image_label_->setText("No Image");
    image_label_->setPixmap(QPixmap());
  }
}

void ClientThumbnailWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    emit clicked(client_id_);
  }
  QWidget::mousePressEvent(event);
}

void ClientThumbnailWidget::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);

  auto lockAction = menu.addAction("Lock");
  connect(lockAction, &QAction::triggered,
          [this]() { emit lockRequested(client_id_); });

  auto unlockAction = menu.addAction("Unlock");
  connect(unlockAction, &QAction::triggered,
          [this]() { emit unlockRequested(client_id_); });

  menu.addSeparator();

  auto screenshotAction = menu.addAction("Request Screenshot");
  connect(screenshotAction, &QAction::triggered,
          [this]() { emit screenshotRequested(client_id_); });

  menu.addSeparator();

  auto disconnectAction = menu.addAction("Disconnect");
  connect(disconnectAction, &QAction::triggered,
          [this]() { emit disconnectRequested(client_id_); });

  menu.exec(event->globalPos());
}

void ClientThumbnailWidget::updateStyle(master::ClientState state) {
  switch (state) {
  case master::ClientState::CONNECTED:
    status_label_->setText("Connected");
    status_label_->setStyleSheet("color: green;");
    image_label_->setStyleSheet(
        "background-color: #333; color: #aaa; border: 2px solid green;");
    break;
  case master::ClientState::LOCKED:
    status_label_->setText("LOCKED");
    status_label_->setStyleSheet("color: orange; font-weight: bold;");
    image_label_->setStyleSheet(
        "background-color: #333; color: #aaa; border: 2px solid orange;");
    break;
  case master::ClientState::DISCONNECTED:
    status_label_->setText("Disconnected");
    status_label_->setStyleSheet("color: red;");
    image_label_->setStyleSheet(
        "background-color: #333; color: #aaa; border: 1px solid #555;");
    break;
  default:
    break;
  }
}

} // namespace ui
} // namespace cms
