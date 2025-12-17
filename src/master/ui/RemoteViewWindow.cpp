#include "cms/ui/RemoteViewWindow.h"
#include <QImage>
#include <QPixmap>
#include <QTimer>


namespace cms {
namespace ui {

RemoteViewWindow::RemoteViewWindow(const std::string &client_id,
                                   std::shared_ptr<master::MasterServer> server,
                                   QWidget *parent)
    : QMainWindow(parent), client_id_(client_id), server_(server) {
  setupUi();

  // Register observer
  observer_ = new StreamObserver(this);
  server_->addObserver(observer_);

  setWindowTitle(
      QString("Remote View - %1").arg(QString::fromStdString(client_id)));
  resize(800, 600);
}

RemoteViewWindow::~RemoteViewWindow() {
  if (server_ && observer_) {
    server_->removeObserver(observer_);
    delete observer_;
  }
}

void RemoteViewWindow::setupUi() {
  scroll_area_ = new QScrollArea(this);
  scroll_area_->setBackgroundRole(QPalette::Dark);
  scroll_area_->setWidgetResizable(
      true); // Fit to window? Or scroll? Let's fit for now with scaled contents

  image_label_ = new QLabel(this);
  image_label_->setAlignment(Qt::AlignCenter);
  image_label_->setText("Waiting for video stream...");
  image_label_->setStyleSheet(
      "QLabel { background-color : black; color : white; }");
  image_label_->setScaledContents(true); // Scale image to label size

  scroll_area_->setWidget(image_label_);
  setCentralWidget(scroll_area_);
}

void RemoteViewWindow::updateImage(const std::vector<uint8_t> &data) {
  if (data.empty())
    return;

  // Assuming RGBA raw data for now, but MasterServer should give us something
  // usable. Ideally MasterServer decodes or provides a QImage-friendly format.
  // For MVP, let's assume raw RGBA 32-bit.
  // But wait, the standard QImage constructor needs width/height.
  // ClientInfo has width/height. For now let's just make a placeholder or try
  // to load if it's a format like PNG/JPG. If it's raw pixel data, we need
  // dimensions.

  // Hack: Try to load as QPixmap (if it's a file format like PNG sent over
  // wire)
  QPixmap pixmap;
  if (pixmap.loadFromData(data.data(), static_cast<uint32_t>(data.size()))) {
    QMetaObject::invokeMethod(
        this, [this, pixmap]() { image_label_->setPixmap(pixmap); },
        Qt::QueuedConnection);
  } else {
    // Fallback for raw data if we knew dimensions.
    // For now, assume it's valid image format (PNG/JPG) as simplified protocol
  }
}

void RemoteViewWindow::closeEvent(QCloseEvent *event) {
  // TODO: Send stop stream command
  QMainWindow::closeEvent(event);
}

} // namespace ui
} // namespace cms
