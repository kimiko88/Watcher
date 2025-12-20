#include "cms/ui/ScreenshotViewerDialog.h"
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace cms {
namespace ui {

ScreenshotViewerDialog::ScreenshotViewerDialog(const QImage &screenshot,
                                               const QString &clientId,
                                               const QString &clientHostname,
                                               QWidget *parent)
    : QDialog(parent), originalImage_(screenshot), clientId_(clientId),
      clientHostname_(clientHostname), zoomLevel_(1.0) {

  setWindowTitle(QString("Screenshot - %1").arg(clientHostname));
  resize(900, 700);

  setupUi();
  updateImageDisplay();
}

ScreenshotViewerDialog::~ScreenshotViewerDialog() {}

void ScreenshotViewerDialog::setupUi() {
  auto mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(12);

  // Info label
  infoLabel_ = new QLabel(this);
  QString timestamp =
      QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
  infoLabel_->setText(
      QString(
          "<b>Client:</b> %1 (%2) | <b>Time:</b> %3 | <b>Resolution:</b> %4x%5")
          .arg(clientHostname_)
          .arg(clientId_)
          .arg(timestamp)
          .arg(originalImage_.width())
          .arg(originalImage_.height()));
  infoLabel_->setWordWrap(true);
  infoLabel_->setStyleSheet(
      "padding: 8px; background-color: #313244; border-radius: 6px;");
  mainLayout->addWidget(infoLabel_);

  // Image display area with scroll
  auto scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setAlignment(Qt::AlignCenter);

  imageLabel_ = new QLabel(scrollArea);
  imageLabel_->setScaledContents(false);
  imageLabel_->setAlignment(Qt::AlignCenter);
  scrollArea->setWidget(imageLabel_);

  mainLayout->addWidget(scrollArea, 1);

  // Zoom controls
  auto zoomLayout = new QHBoxLayout();

  zoomOutButton_ = new QPushButton("🔍−", this);
  zoomOutButton_->setFixedWidth(50);
  zoomOutButton_->setToolTip("Zoom Out");
  connect(zoomOutButton_, &QPushButton::clicked, this,
          &ScreenshotViewerDialog::zoomOut);
  zoomLayout->addWidget(zoomOutButton_);

  zoomSlider_ = new QSlider(Qt::Horizontal, this);
  zoomSlider_->setRange(25, 400); // 25% to 400%
  zoomSlider_->setValue(100);     // 100% default
  zoomSlider_->setTickPosition(QSlider::TicksBelow);
  zoomSlider_->setTickInterval(25);
  connect(zoomSlider_, &QSlider::valueChanged, this,
          &ScreenshotViewerDialog::onZoomSliderChanged);
  zoomLayout->addWidget(zoomSlider_, 1);

  zoomInButton_ = new QPushButton("🔍+", this);
  zoomInButton_->setFixedWidth(50);
  zoomInButton_->setToolTip("Zoom In");
  connect(zoomInButton_, &QPushButton::clicked, this,
          &ScreenshotViewerDialog::zoomIn);
  zoomLayout->addWidget(zoomInButton_);

  zoomResetButton_ = new QPushButton("1:1", this);
  zoomResetButton_->setFixedWidth(50);
  zoomResetButton_->setToolTip("Reset Zoom");
  connect(zoomResetButton_, &QPushButton::clicked, this,
          &ScreenshotViewerDialog::zoomReset);
  zoomLayout->addWidget(zoomResetButton_);

  mainLayout->addLayout(zoomLayout);

  // Action buttons
  auto buttonLayout = new QHBoxLayout();
  buttonLayout->setSpacing(8);

  saveButton_ = new QPushButton("💾 Save Screenshot", this);
  saveButton_->setMinimumWidth(150);
  connect(saveButton_, &QPushButton::clicked, this,
          &ScreenshotViewerDialog::saveScreenshot);
  buttonLayout->addWidget(saveButton_);

  buttonLayout->addStretch();

  closeButton_ = new QPushButton("Close", this);
  closeButton_->setMinimumWidth(100);
  closeButton_->setProperty("class", "secondary");
  connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
  buttonLayout->addWidget(closeButton_);

  mainLayout->addLayout(buttonLayout);
}

void ScreenshotViewerDialog::updateImageDisplay() {
  if (originalImage_.isNull()) {
    imageLabel_->setText("No screenshot available");
    return;
  }

  int width = originalImage_.width() * zoomLevel_;
  int height = originalImage_.height() * zoomLevel_;

  QPixmap pixmap = QPixmap::fromImage(originalImage_);
  imageLabel_->setPixmap(pixmap.scaled(width, height, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation));
}

void ScreenshotViewerDialog::saveScreenshot() {
  QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
  QString defaultFilename = QString("screenshot_%1_%2.png")
                                .arg(clientHostname_.replace(' ', '_'))
                                .arg(timestamp);

  // Get Pictures/Screenshots directory as default location
  QString picturesPath =
      QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
  QString screenshotsPath = picturesPath + "/Screenshots";

  // Create Screenshots directory if it doesn't exist
  QDir dir;
  if (!dir.exists(screenshotsPath)) {
    dir.mkpath(screenshotsPath);
  }

  QString defaultPath = screenshotsPath + "/" + defaultFilename;

  QString filename = QFileDialog::getSaveFileName(
      this, "Save Screenshot", defaultPath,
      "PNG Images (*.png);;JPEG Images (*.jpg);;All Files (*.*)");

  if (!filename.isEmpty()) {
    if (originalImage_.save(filename)) {
      QMessageBox::information(
          this, "Success", QString("Screenshot saved to:\n%1").arg(filename));
    } else {
      QMessageBox::critical(this, "Error", "Failed to save screenshot!");
    }
  }
}

void ScreenshotViewerDialog::zoomIn() {
  int newValue = qMin(400, zoomSlider_->value() + 25);
  zoomSlider_->setValue(newValue);
}

void ScreenshotViewerDialog::zoomOut() {
  int newValue = qMax(25, zoomSlider_->value() - 25);
  zoomSlider_->setValue(newValue);
}

void ScreenshotViewerDialog::zoomReset() { zoomSlider_->setValue(100); }

void ScreenshotViewerDialog::onZoomSliderChanged(int value) {
  zoomLevel_ = value / 100.0;
  updateImageDisplay();
}

} // namespace ui
} // namespace cms
