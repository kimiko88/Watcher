#include "cms/ui/MasterWindow.h"
#include "cms/ui/ClientThumbnailWidget.h"
#include "cms/ui/RemoteViewWindow.h"
#include <QAction>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <iostream>

namespace cms {
namespace ui {

MasterWindow::MasterWindow(std::shared_ptr<master::MasterServer> server,
                           QWidget *parent)
    : QMainWindow(parent), server_(server) {
  model_ = new ClientListModel(this);

  setupUi();
  setupToolbar();

  server_->addObserver(model_);

  // Connect model changes to grid updates
  connect(model_, &QAbstractTableModel::rowsInserted, this,
          &MasterWindow::refreshGrid);
  connect(model_, &QAbstractTableModel::modelReset, this,
          &MasterWindow::refreshGrid);

  resize(1024, 768);
  setWindowTitle("Classroom Management System - Master");

  // DEBUG: Simulate a client connection after 2 seconds
  QTimer::singleShot(2000, [this]() {
    // This is a hack because we know the implementation details.
    // In real code, we wouldn't cast. This is just to prove the UI works.
    // Dynamic_cast would require RTTI enabled everywhere.
    // Let's assume we can somehow trigger simulation or wait for real
    // connection.
  });
}

MasterWindow::~MasterWindow() {
  if (server_) {
    server_->removeObserver(model_);
  }
}

void MasterWindow::setupUi() {
  central_widget_ = new QWidget(this);
  auto mainLayout = new QVBoxLayout(central_widget_);

  scroll_area_ = new QScrollArea(this);
  scroll_area_->setWidgetResizable(true);

  grid_container_ = new QWidget(scroll_area_);
  // Fallback if no FlowLayout: Use QGridLayout with fixed columns
  // actually let's use QGridLayout for simplicity MVP
  auto grid = new QGridLayout(grid_container_);
  grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

  grid_container_->setLayout(grid);
  scroll_area_->setWidget(grid_container_);

  mainLayout->addWidget(scroll_area_);
  setCentralWidget(central_widget_);

  statusBar()->showMessage("Ready");
}

void MasterWindow::refreshGrid() {
  // Clear existing items provided they are widgets
  QLayoutItem *child;
  auto layout = grid_container_->layout();
  while ((child = layout->takeAt(0)) != nullptr) {
    if (child->widget()) {
      delete child->widget();
    }
    delete child;
  }

  // Rebuild grid
  auto *grid = static_cast<QGridLayout *>(layout);
  int columns = 3; // Fixed columns for now

  for (int i = 0; i < model_->rowCount(); ++i) {
    auto client = model_->getClient(i);
    auto widget = new ClientThumbnailWidget(grid_container_);
    widget->setClientInfo(client);
    connect(widget, &ClientThumbnailWidget::clicked, this,
            &MasterWindow::onClientThumbnailClicked);

    int row = i / columns;
    int col = i % columns;
    grid->addWidget(widget, row, col);
  }

  statusBar()->showMessage(QString("Clients: %1").arg(model_->rowCount()));
}

void MasterWindow::setupToolbar() {
  auto toolbar = addToolBar("Main Toolbar");
  toolbar->setMovable(false);

  auto lockAction = toolbar->addAction("Lock All");
  connect(lockAction, &QAction::triggered, this, &MasterWindow::onLockAll);

  auto unlockAction = toolbar->addAction("Unlock All");
  connect(unlockAction, &QAction::triggered, this, &MasterWindow::onUnlockAll);

  toolbar->addSeparator();

  auto refreshAction = toolbar->addAction("Refresh");
  connect(refreshAction, &QAction::triggered, this, &MasterWindow::onRefresh);

  auto debugAddClient = toolbar->addAction("[DEBUG] Add Client");
  connect(debugAddClient, &QAction::triggered, [this]() {
    static int count = 1;
    master::ClientInfo info;
    info.id = "debug_" + std::to_string(count);
    info.hostname = "Student-" + std::to_string(count);
    info.ip_address = "192.168.1.10" + std::to_string(count);
    info.state = master::ClientState::CONNECTED;
    model_->onClientConnected(info); // Directly call observer method for debug
    count++;
  });
}

void MasterWindow::onRefresh() {
  statusBar()->showMessage("Refreshing...");
  // Trigger server discovery or refresh
}

void MasterWindow::onLockAll() {
  server_->lockAll();
  statusBar()->showMessage("Locked all clients");
}

void MasterWindow::onUnlockAll() {
  server_->unlockAll();
  statusBar()->showMessage("Unlocked all clients");
}

void MasterWindow::onClientThumbnailClicked(const std::string &client_id) {
  if (remote_views_.find(client_id) != remote_views_.end()) {
    remote_views_[client_id]->raise();
    remote_views_[client_id]->activateWindow();
    return;
  }

  auto *window = new RemoteViewWindow(client_id, server_, this);
  window->setAttribute(Qt::WA_DeleteOnClose);

  connect(window, &QWidget::destroyed,
          [this, client_id]() { remote_views_.erase(client_id); });

  remote_views_[client_id] = window;
  window->show();
}

} // namespace ui
} // namespace cms
