#ifndef CMS_UI_SCREENSHOT_VIEWER_DIALOG_H
#define CMS_UI_SCREENSHOT_VIEWER_DIALOG_H

#include <QDialog>
#include <QImage>
#include <QString>

class QLabel;
class QPushButton;
class QSlider;

namespace cms {
namespace ui {

/**
 * Dialog for viewing screenshots captured from client machines
 * Features: zoom in/out, save to file, full image display
 */
class ScreenshotViewerDialog : public QDialog {
  Q_OBJECT

public:
  explicit ScreenshotViewerDialog(const QImage &screenshot,
                                  const QString &clientId,
                                  const QString &clientHostname,
                                  QWidget *parent = nullptr);
  ~ScreenshotViewerDialog();

public slots:
  void saveScreenshot();
  void zoomIn();
  void zoomOut();
  void zoomReset();

private slots:
  void onZoomSliderChanged(int value);

private:
  void setupUi();
  void updateImageDisplay();

  // Data
  QImage originalImage_;
  QString clientId_;
  QString clientHostname_;
  double zoomLevel_;

  // UI Components
  QLabel *imageLabel_;
  QLabel *infoLabel_;
  QPushButton *saveButton_;
  QPushButton *zoomInButton_;
  QPushButton *zoomOutButton_;
  QPushButton *zoomResetButton_;
  QPushButton *closeButton_;
  QSlider *zoomSlider_;
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_SCREENSHOT_VIEWER_DIALOG_H
