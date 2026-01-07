#include "cms/ui/RemoteViewWindow.h"
#include <QCloseEvent>
#include <QLabel>
#include <QMouseEvent> // Added for mousePressEvent
#include <QScrollArea>
#include <QVBoxLayout>

// Removed:
// #include <QImage>
// #include <QPixmap>
// #include <QTimer>

namespace cms {
namespace ui {

RemoteViewWindow::RemoteViewWindow(
    const std::string &client_id,
    std::shared_ptr<cms::master::MasterServer> server, QWidget *parent)
    : QMainWindow(parent), client_id_(client_id), server_(server) {
  setupUi(); // Call setupUi here

  // Register observer - Removed
  // observer_ = new StreamObserver(this);
  // server_->addObserver(observer_);

  // Removed:
  // setWindowTitle(
  //     QString("Remote View - %1").arg(QString::fromStdString(client_id)));
  // resize(800, 600);

  // New comments from edit:
  // In a real app we might register strictly for this client
  // server_->addObserver(this); // Requires us to implement IServerObserver or
  // use a proxy For simplicity, MasterWindow updates us, or we just pull? Let's
  // assume MasterWindow calls updateImage() on us.
}

RemoteViewWindow::~RemoteViewWindow() {
  // server_->removeObserver(this); // invalid
}

bool RemoteViewWindow::eventFilter(QObject *obj, QEvent *event) {
  if (obj == image_label_) {
    if (event->type() == QEvent::MouseMove) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      if (image_label_->width() > 0 && image_label_->height() > 0) {
        float x = static_cast<float>(mouseEvent->x()) / image_label_->width();
        float y = static_cast<float>(mouseEvent->y()) / image_label_->height();

        nlohmann::json payload;
        payload["type"] = "mouse_move";
        payload["x"] = x;
        payload["y"] = y;
        server_->sendRemoteInput(client_id_, payload);
      }
      return true;
    } else if (event->type() == QEvent::MouseButtonPress ||
               event->type() == QEvent::MouseButtonRelease) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      if (image_label_->width() > 0 && image_label_->height() > 0) {
        float x = static_cast<float>(mouseEvent->x()) / image_label_->width();
        float y = static_cast<float>(mouseEvent->y()) / image_label_->height();
        bool isDown = (event->type() == QEvent::MouseButtonPress);
        bool isLeft = (mouseEvent->button() == Qt::LeftButton);

        nlohmann::json payload;
        payload["type"] = "mouse_click";
        payload["x"] = x;
        payload["y"] = y;
        payload["left"] = isLeft;
        payload["down"] = isDown;
        server_->sendRemoteInput(client_id_, payload);
      }
      return true;
    } else if (event->type() == QEvent::KeyPress ||
               event->type() == QEvent::KeyRelease) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      bool isDown = (event->type() == QEvent::KeyPress);
      int nativeKey = keyEvent->nativeVirtualKey();

      nlohmann::json payload;
      payload["type"] = "key";
      payload["key"] = nativeKey;
      payload["down"] = isDown;
      server_->sendRemoteInput(client_id_, payload);
      return true;
    }
  }
  return QMainWindow::eventFilter(obj, event);
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

  image_label_->setMouseTracking(true);
  image_label_->installEventFilter(this);
}

void RemoteViewWindow::updateImage(const std::vector<uint8_t> &data, int width,
                                   int height) {
  if (data.empty())
    return;

  // We have raw RGBA data and dimensions. Create QImage.
  if (width > 0 && height > 0 &&
      data.size() >= static_cast<size_t>(width * height * 4)) {
    QImage img((const uchar *)data.data(), width, height,
               QImage::Format_RGBA8888);
    // QImage references the data, so we must make a deep copy or turn into
    // Pixmap immediately
    QPixmap pixmap = QPixmap::fromImage(img);

    QMetaObject::invokeMethod(
        this,
        [this, pixmap]() {
          image_label_->setPixmap(pixmap);
          // Adjust window size or scroll area if needed, but for now just show
          // it
        },
        Qt::QueuedConnection);
    return;
  }

  // Fallback if no dimensions (legacy fallback)
  QPixmap pixmap;
  if (pixmap.loadFromData(data.data(), static_cast<uint32_t>(data.size()))) {
    QMetaObject::invokeMethod(
        this, [this, pixmap]() { image_label_->setPixmap(pixmap); },
        Qt::QueuedConnection);
  }
}

void RemoteViewWindow::closeEvent(QCloseEvent *event) {
  // TODO: Send stop stream command
  QMainWindow::closeEvent(event);
}

} // namespace ui
} // namespace cms
