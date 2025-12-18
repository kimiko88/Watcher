#include "cms/ui/MasterWindow.h"
#include "cms/ui/ClientThumbnailWidget.h"
#include "cms/ui/DomainPolicyDialog.h"
#include "cms/ui/RemoteViewWindow.h"
#include "cms/ui/ScreenshotViewerDialog.h"
#include <QAction>
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
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
  setupMenuBar();
  setupToolbar();

  server_->addObserver(model_);
  server_->addObserver(this);

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
    server_->removeObserver(this);
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

void MasterWindow::setupMenuBar() {
  auto fileMenu = menuBar()->addMenu("&File");
  auto exitAction = fileMenu->addAction("E&xit");
  exitAction->setShortcut(QKeySequence::Quit);
  connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

  auto viewMenu = menuBar()->addMenu("&View");
  auto refreshAction = viewMenu->addAction("&Refresh");
  refreshAction->setShortcut(QKeySequence::Refresh);
  connect(refreshAction, &QAction::triggered, this, &MasterWindow::onRefresh);

  auto toolsMenu = menuBar()->addMenu("&Tools");
  auto screenshotAction = toolsMenu->addAction("Take &Screenshot");
  screenshotAction->setShortcut(QKeySequence("Ctrl+S"));
  connect(screenshotAction, &QAction::triggered, this,
          &MasterWindow::onTakeScreenshot);

  toolsMenu->addSeparator();
  auto domainPolicyAction = toolsMenu->addAction("&Domain Policy...");
  connect(domainPolicyAction, &QAction::triggered, this,
          &MasterWindow::onDomainPolicyClicked);

  auto appPolicyAction = toolsMenu->addAction("&Application Policy...");
  connect(appPolicyAction, &QAction::triggered, this,
          &MasterWindow::onApplicationPolicyClicked);

  toolsMenu->addSeparator();
  auto lockAllAction = toolsMenu->addAction("&Lock All Clients");
  lockAllAction->setShortcut(QKeySequence("Ctrl+L"));
  connect(lockAllAction, &QAction::triggered, this, &MasterWindow::onLockAll);

  auto unlockAllAction = toolsMenu->addAction("&Unlock All Clients");
  unlockAllAction->setShortcut(QKeySequence("Ctrl+U"));
  connect(unlockAllAction, &QAction::triggered, this,
          &MasterWindow::onUnlockAll);

  auto helpMenu = menuBar()->addMenu("&Help");
  auto aboutAction = helpMenu->addAction("&About");
  connect(aboutAction, &QAction::triggered, this,
          &MasterWindow::onAboutClicked);
}

void MasterWindow::setupToolbar() {
  auto toolbar = addToolBar("Main Toolbar");
  toolbar->setMovable(false);

  auto lockAction = toolbar->addAction("Lock All");
  connect(lockAction, &QAction::triggered, this, &MasterWindow::onLockAll);

  auto unlockAction = toolbar->addAction("Unlock All");
  connect(unlockAction, &QAction::triggered, this, &MasterWindow::onUnlockAll);

  toolbar->addSeparator();

  auto screenshotAction = toolbar->addAction("📸 Screenshot");
  screenshotAction->setToolTip("Take screenshot from selected client");
  connect(screenshotAction, &QAction::triggered, this,
          &MasterWindow::onTakeScreenshot);

  toolbar->addSeparator();

  auto domainPolicyAction = toolbar->addAction("🌐 Domains");
  domainPolicyAction->setToolTip("Configure domain policy");
  connect(domainPolicyAction, &QAction::triggered, this,
          &MasterWindow::onDomainPolicyClicked);

  auto appPolicyAction = toolbar->addAction("🎮 Apps");
  appPolicyAction->setToolTip("Configure application policy");
  connect(appPolicyAction, &QAction::triggered, this,
          &MasterWindow::onApplicationPolicyClicked);

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

void MasterWindow::onTakeScreenshot() {
  if (model_->rowCount() == 0) {
    QMessageBox::information(
        this, "No Clients",
        "No clients connected. Please connect clients first.");
    return;
  }

  // Request from first client for now (MVP)
  // TODO: Get selected client from grid
  auto client = model_->getClient(0);
  server_->refreshClientThumbnail(client.id);
  statusBar()->showMessage("Requested screenshot from " +
                           QString::fromStdString(client.hostname));
}

void MasterWindow::onDomainPolicyClicked() {
  // TODO: Get actual policy from server
  cms::DomainPolicy policy;

  DomainPolicyDialog dialog(policy, this);
  if (dialog.exec() == QDialog::Accepted) {
    // Save policy to server and broadcast
    cms::DomainPolicy newPolicy = dialog.getPolicy();

    // Broadcast to all clients
    auto clients = server_->getConnectedClients();
    int count = 0;
    for (const auto &client : clients) {
      if (server_->sendDomainPolicy(client.id, newPolicy)) {
        count++;
      }
    }

    statusBar()->showMessage(
        QString("Domain policy sent to %1 clients").arg(count));
  }
}

void MasterWindow::onApplicationPolicyClicked() {
  QMessageBox::information(this, "Application Policy",
                           "Application Policy management coming soon.");
}

void MasterWindow::onAboutClicked() {
  QMessageBox::about(this, "About", "Classroom Management System\nVersion 1.0");
}

void MasterWindow::onScreenshotReceived(const std::string &client_id,
                                        const std::vector<uint8_t> &data) {
  QImage image;
  if (image.loadFromData(data.data(), static_cast<int>(data.size()))) {
    auto client = server_->getClientInfo(client_id);

    // Check if we already have a dialog for this client?
    // For now, just open a new one.

    ScreenshotViewerDialog *dialog = new ScreenshotViewerDialog(
        image, QString::fromStdString(client.id),
        QString::fromStdString(client.hostname), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    statusBar()->showMessage("Screenshot received from " +
                             QString::fromStdString(client.hostname));
  } else {
    statusBar()->showMessage("Failed to load screenshot data");
  }
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
