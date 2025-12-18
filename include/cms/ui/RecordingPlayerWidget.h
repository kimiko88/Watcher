#pragma once

#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QWidget>
#include <cstdint>
#include <string>

namespace cms {
namespace ui {

// Qt Widget for playing back recorded videos using FFmpeg
class RecordingPlayerWidget : public QWidget {
  Q_OBJECT

public:
  explicit RecordingPlayerWidget(QWidget *parent = nullptr);
  ~RecordingPlayerWidget();

  // Load and prepare a recording for playback
  // @param filePath: Absolute path to the video file
  // @return true if loaded successfully
  bool loadRecording(const QString &filePath);

  // Get the duration of the loaded recording in milliseconds
  // @return Duration in milliseconds, or 0 if no recording loaded
  int64_t getDuration() const;

  // Get the current playback position in milliseconds
  // @return Current position in milliseconds
  int64_t getPosition() const;

  // Check if currently playing
  // @return true if playing, false otherwise
  bool isPlaying() const;

public slots:
  // Start or resume playback
  void play();

  // Pause playback
  void pause();

  // Stop playback and reset to beginning
  void stop();

  // Seek to a specific timestamp
  // @param timestampMs: Position in milliseconds
  void seek(int64_t timestampMs);

  // Set playback volume (0-100)
  // @param volume: Volume level
  void setVolume(int volume);

signals:
  // Emitted when playback starts
  void playbackStarted();

  // Emitted when playback is paused
  void playbackPaused();

  // Emitted when playback stops
  void playbackStopped();

  // Emitted when playback reaches the end
  void playbackFinished();

  // Emitted when the playback position changes
  // @param timestampMs: Current position in milliseconds
  void positionChanged(int64_t timestampMs);

  // Emitted when a recording is loaded
  // @param durationMs: Total duration in milliseconds
  void durationChanged(int64_t durationMs);

  // Emitted when an error occurs
  // @param errorMessage: Description of the error
  void errorOccurred(const QString &errorMessage);

private:
  // Prevent copying
  RecordingPlayerWidget(const RecordingPlayerWidget &) = delete;
  RecordingPlayerWidget &operator=(const RecordingPlayerWidget &) = delete;

  // UI components
  QLabel *videoLabel_;
  QPushButton *playButton_;
  QPushButton *pauseButton_;
  QPushButton *stopButton_;
  QSlider *timelineSlider_;
  QLabel *timeLabel_;
  QSlider *volumeSlider_;

  // Setup the UI layout
  void setupUI();

  // Update the time label display
  void updateTimeLabel();

  // Format milliseconds to MM:SS string
  QString formatTime(int64_t milliseconds) const;

  // Private implementation (pimpl pattern for FFmpeg dependencies)
  class Impl;
  Impl *pImpl_;

private slots:
  // Handle timeline slider changes from user
  void onTimelineSliderMoved(int position);

  // Handle volume slider changes
  void onVolumeSliderMoved(int volume);

  // Update UI from playback thread
  void onFrameReady();
};

} // namespace ui
} // namespace cms
