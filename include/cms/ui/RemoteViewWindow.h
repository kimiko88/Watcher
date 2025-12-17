#ifndef CMS_UI_REMOTE_VIEW_WINDOW_H
#define CMS_UI_REMOTE_VIEW_WINDOW_H

#include "cms/MasterServer.h"
#include <QLabel>
#include <QMainWindow>
#include <QResizeEvent>
#include <QScrollArea>
#include <memory>


namespace cms {
namespace ui {

class RemoteViewWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit RemoteViewWindow(const std::string &client_id,
                            std::shared_ptr<master::MasterServer> server,
                            QWidget *parent = nullptr);
  ~RemoteViewWindow() override;

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  void setupUi();
  void updateImage(const std::vector<uint8_t> &data);

  std::string client_id_;
  std::shared_ptr<master::MasterServer> server_;

  QLabel *image_label_;
  QScrollArea *scroll_area_;

  // Observer to listen for stream updates
  class StreamObserver : public master::MasterServer::IServerObserver {
  public:
    StreamObserver(RemoteViewWindow *window) : window_(window) {}
    void onClientThumbnailUpdated(const std::string &client_id,
                                  const std::vector<uint8_t> &data) override {
      if (client_id == window_->client_id_) {
        window_->updateImage(data);
      }
    }
    // Other methods empty
  private:
    RemoteViewWindow *window_;
  };

  StreamObserver *observer_ = nullptr;
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_REMOTE_VIEW_WINDOW_H
