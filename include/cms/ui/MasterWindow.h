#ifndef CMS_UI_MASTER_WINDOW_H
#define CMS_UI_MASTER_WINDOW_H

#include "cms/MasterServer.h"
#include "cms/ui/ClientListModel.h"
#include <QMainWindow>
#include <map>
#include <string>

class QListView;
class QScrollArea;
class QGridView; // Placeholder if we make a custom grid view

namespace cms {
namespace ui {

class MasterWindow : public QMainWindow,
                     public master::MasterServer::IServerObserver {
  Q_OBJECT

public:
  explicit MasterWindow(std::shared_ptr<master::MasterServer> server,
                        QWidget *parent = nullptr);
  ~MasterWindow();

  // IServerObserver overrides
  void onScreenshotReceived(const std::string &client_id,
                            const std::vector<uint8_t> &data) override;

private slots:
  void onRefresh();
  void onLockAll();
  void onUnlockAll();
  void onTakeScreenshot();
  void onDomainPolicyClicked();
  void onApplicationPolicyClicked();
  void onAboutClicked();

private:
  std::shared_ptr<master::MasterServer> server_;
  ClientListModel *model_;

  // UI Elements
  QWidget *central_widget_;
  QScrollArea *scroll_area_;
  QWidget *grid_container_;
  // QLayout* grid_layout_; // Not strictly needed as member if we use local
  // var, but good for cleanup logic if needed

  void setupUi();
  void setupMenuBar();
  void setupToolbar();
  void refreshGrid();
  void updateStatusBar();

private slots:
  void onClientThumbnailClicked(const std::string &client_id);

private:
  std::map<std::string, class RemoteViewWindow *> remote_views_;
  std::string selected_client_id_;
  cms::DomainPolicy current_policy_;
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_MASTER_WINDOW_H
