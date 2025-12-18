#include "cms/ui/RecordingPlayerWidget.h"
#include <QHBoxLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <chrono>
#include <mutex>
#include <thread>

#ifdef CMS_HAS_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#endif

namespace cms {
namespace ui {

// Private implementation class
class RecordingPlayerWidget::Impl {
public:
  std::string filename;
  bool playing = false;
  bool loaded = false;
  int64_t duration_ms = 0;
  int64_t current_position_ms = 0;
  int volume = 50;

#ifdef CMS_HAS_FFMPEG
  AVFormatContext *format_ctx = nullptr;
  AVCodecContext *codec_ctx = nullptr;
  AVFrame *frame = nullptr;
  AVPacket *packet = nullptr;
  SwsContext *sws_ctx = nullptr;
  int video_stream_index = -1;
#endif

  std::thread playback_thread;
  std::mutex mutex;
  bool stop_requested = false;

  QTimer *update_timer = nullptr;
  QImage current_frame;

  Impl() {
#ifdef CMS_HAS_FFMPEG
    // Initialize FFmpeg (deprecated in newer versions but harmless)
    av_register_all();
#endif
  }

  ~Impl() { cleanup(); }

  bool load(const std::string &path) {
    cleanup();

#ifdef CMS_HAS_FFMPEG
    // Open video file
    if (avformat_open_input(&format_ctx, path.c_str(), nullptr, nullptr) != 0) {
      return false;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
      cleanup();
      return false;
    }

    // Find the video stream
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
      if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_stream_index = i;
        break;
      }
    }

    if (video_stream_index == -1) {
      cleanup();
      return false;
    }

    // Get codec parameters
    AVCodecParameters *codec_params =
        format_ctx->streams[video_stream_index]->codecpar;

    // Find decoder
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
      cleanup();
      return false;
    }

    // Create codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
      cleanup();
      return false;
    }

    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
      cleanup();
      return false;
    }

    // Open codec
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
      cleanup();
      return false;
    }

    // Allocate frame and packet
    frame = av_frame_alloc();
    packet = av_packet_alloc();

    if (!frame || !packet) {
      cleanup();
      return false;
    }

    // Calculate duration
    if (format_ctx->duration != AV_NOPTS_VALUE) {
      duration_ms = (format_ctx->duration * 1000) / AV_TIME_BASE;
    }

    filename = path;
    loaded = true;
    current_position_ms = 0;

    return true;
#else
    // Stub implementation without FFmpeg
    filename = path;
    loaded = true;
    duration_ms = 10000; // 10 seconds fake duration
    current_position_ms = 0;
    return true;
#endif
  }

  void cleanup() {
#ifdef CMS_HAS_FFMPEG
    if (packet) {
      av_packet_free(&packet);
      packet = nullptr;
    }
    if (frame) {
      av_frame_free(&frame);
      frame = nullptr;
    }
    if (sws_ctx) {
      sws_freeContext(sws_ctx);
      sws_ctx = nullptr;
    }
    if (codec_ctx) {
      avcodec_free_context(&codec_ctx);
      codec_ctx = nullptr;
    }
    if (format_ctx) {
      avformat_close_input(&format_ctx);
      format_ctx = nullptr;
    }
    video_stream_index = -1;
#endif
    loaded = false;
    playing = false;
  }

  void startPlayback() {
    if (!loaded || playing) {
      return;
    }

    playing = true;
    stop_requested = false;

    // Start playback in a separate thread
    playback_thread = std::thread([this]() { playbackLoop(); });
  }

  void stopPlayback() {
    if (!playing) {
      return;
    }

    stop_requested = true;
    playing = false;

    if (playback_thread.joinable()) {
      playback_thread.join();
    }
  }

  void pausePlayback() {
    std::lock_guard<std::mutex> lock(mutex);
    playing = false;
  }

  void seekTo(int64_t position_ms) {
#ifdef CMS_HAS_FFMPEG
    if (!loaded || !format_ctx) {
      return;
    }

    int64_t timestamp = (position_ms * AV_TIME_BASE) / 1000;

    if (av_seek_frame(format_ctx, -1, timestamp, AVSEEK_FLAG_BACKWARD) >= 0) {
      avcodec_flush_buffers(codec_ctx);
      current_position_ms = position_ms;
    }
#else
    current_position_ms = position_ms;
#endif
  }

private:
  void playbackLoop() {
#ifdef CMS_HAS_FFMPEG
    while (!stop_requested && loaded) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        if (!playing) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          continue;
        }
      }

      // Read frame
      if (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
          if (avcodec_send_packet(codec_ctx, packet) == 0) {
            if (avcodec_receive_frame(codec_ctx, frame) == 0) {
              // Convert frame to RGB for Qt display
              convertFrameToQImage();

              // Update position
              if (frame->pts != AV_NOPTS_VALUE) {
                AVStream *stream = format_ctx->streams[video_stream_index];
                current_position_ms =
                    (frame->pts * 1000 * stream->time_base.num) /
                    stream->time_base.den;
              }

              // Simulate real-time playback
              std::this_thread::sleep_for(
                  std::chrono::milliseconds(33)); // ~30fps
            }
          }
        }
        av_packet_unref(packet);
      } else {
        // End of file
        stop_requested = true;
        playing = false;
      }
    }
#else
    // Stub playback loop
    while (!stop_requested && loaded && playing) {
      current_position_ms += 100;
      if (current_position_ms >= duration_ms) {
        playing = false;
        stop_requested = true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
  }

#ifdef CMS_HAS_FFMPEG
  void convertFrameToQImage() {
    if (!frame || !codec_ctx) {
      return;
    }

    // Initialize scaling context if needed
    if (!sws_ctx) {
      sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height,
                               codec_ctx->pix_fmt, codec_ctx->width,
                               codec_ctx->height, AV_PIX_FMT_RGB24,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    // Allocate RGB frame
    AVFrame *rgb_frame = av_frame_alloc();
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_ctx->width,
                                             codec_ctx->height, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));

    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer,
                         AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height,
                         1);

    // Convert to RGB
    sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height,
              rgb_frame->data, rgb_frame->linesize);

    // Create QImage
    current_frame =
        QImage(rgb_frame->data[0], codec_ctx->width, codec_ctx->height,
               rgb_frame->linesize[0], QImage::Format_RGB888)
            .copy();

    av_free(buffer);
    av_frame_free(&rgb_frame);
  }
#endif
};

// Public interface implementation
RecordingPlayerWidget::RecordingPlayerWidget(QWidget *parent)
    : QWidget(parent), pImpl_(new Impl()) {
  setupUI();

  // Setup update timer
  pImpl_->update_timer = new QTimer(this);
  connect(pImpl_->update_timer, &QTimer::timeout, this,
          &RecordingPlayerWidget::onFrameReady);
  pImpl_->update_timer->start(33); // ~30 FPS update
}

RecordingPlayerWidget::~RecordingPlayerWidget() {
  pImpl_->stopPlayback();
  delete pImpl_;
}

void RecordingPlayerWidget::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // Video display area
  videoLabel_ = new QLabel(this);
  videoLabel_->setMinimumSize(640, 480);
  videoLabel_->setStyleSheet("QLabel { background-color: black; }");
  videoLabel_->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(videoLabel_);

  // Timeline slider
  timelineSlider_ = new QSlider(Qt::Horizontal, this);
  timelineSlider_->setMinimum(0);
  timelineSlider_->setMaximum(1000);
  connect(timelineSlider_, &QSlider::sliderMoved, this,
          &RecordingPlayerWidget::onTimelineSliderMoved);
  mainLayout->addWidget(timelineSlider_);

  // Time label
  timeLabel_ = new QLabel("00:00 / 00:00", this);
  timeLabel_->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(timeLabel_);

  // Control buttons
  QHBoxLayout *controlLayout = new QHBoxLayout();

  playButton_ = new QPushButton("Play", this);
  connect(playButton_, &QPushButton::clicked, this,
          &RecordingPlayerWidget::play);
  controlLayout->addWidget(playButton_);

  pauseButton_ = new QPushButton("Pause", this);
  connect(pauseButton_, &QPushButton::clicked, this,
          &RecordingPlayerWidget::pause);
  controlLayout->addWidget(pauseButton_);

  stopButton_ = new QPushButton("Stop", this);
  connect(stopButton_, &QPushButton::clicked, this,
          &RecordingPlayerWidget::stop);
  controlLayout->addWidget(stopButton_);

  // Volume control
  volumeSlider_ = new QSlider(Qt::Horizontal, this);
  volumeSlider_->setMinimum(0);
  volumeSlider_->setMaximum(100);
  volumeSlider_->setValue(50);
  volumeSlider_->setMaximumWidth(100);
  connect(volumeSlider_, &QSlider::sliderMoved, this,
          &RecordingPlayerWidget::onVolumeSliderMoved);
  controlLayout->addWidget(new QLabel("Volume:", this));
  controlLayout->addWidget(volumeSlider_);

  mainLayout->addLayout(controlLayout);

  setLayout(mainLayout);
}

bool RecordingPlayerWidget::loadRecording(const QString &filePath) {
  bool success = pImpl_->load(filePath.toStdString());

  if (success) {
    timelineSlider_->setMaximum(static_cast<int>(pImpl_->duration_ms));
    updateTimeLabel();
    emit durationChanged(pImpl_->duration_ms);
  } else {
    emit errorOccurred("Failed to load recording: " + filePath);
  }

  return success;
}

int64_t RecordingPlayerWidget::getDuration() const {
  return pImpl_->duration_ms;
}

int64_t RecordingPlayerWidget::getPosition() const {
  return pImpl_->current_position_ms;
}

bool RecordingPlayerWidget::isPlaying() const { return pImpl_->playing; }

void RecordingPlayerWidget::play() {
  if (!pImpl_->loaded) {
    return;
  }

  pImpl_->startPlayback();
  emit playbackStarted();
}

void RecordingPlayerWidget::pause() {
  pImpl_->pausePlayback();
  emit playbackPaused();
}

void RecordingPlayerWidget::stop() {
  pImpl_->stopPlayback();
  pImpl_->current_position_ms = 0;
  pImpl_->seekTo(0);
  updateTimeLabel();
  emit playbackStopped();
}

void RecordingPlayerWidget::seek(int64_t timestampMs) {
  pImpl_->seekTo(timestampMs);
  updateTimeLabel();
  emit positionChanged(timestampMs);
}

void RecordingPlayerWidget::setVolume(int volume) {
  pImpl_->volume = volume;
  volumeSlider_->setValue(volume);
}

void RecordingPlayerWidget::onTimelineSliderMoved(int position) {
  seek(static_cast<int64_t>(position));
}

void RecordingPlayerWidget::onVolumeSliderMoved(int volume) {
  setVolume(volume);
}

void RecordingPlayerWidget::onFrameReady() {
  // Update timeline
  if (pImpl_->playing) {
    timelineSlider_->setValue(static_cast<int>(pImpl_->current_position_ms));
    updateTimeLabel();
    emit positionChanged(pImpl_->current_position_ms);
  }

  // Update video frame
  if (!pImpl_->current_frame.isNull()) {
    QPixmap pixmap = QPixmap::fromImage(pImpl_->current_frame);
    videoLabel_->setPixmap(pixmap.scaled(
        videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }

  // Check if playback finished
  if (pImpl_->stop_requested && !pImpl_->playing) {
    emit playbackFinished();
  }
}

void RecordingPlayerWidget::updateTimeLabel() {
  QString current = formatTime(pImpl_->current_position_ms);
  QString total = formatTime(pImpl_->duration_ms);
  timeLabel_->setText(current + " / " + total);
}

QString RecordingPlayerWidget::formatTime(int64_t milliseconds) const {
  int64_t seconds = milliseconds / 1000;
  int64_t minutes = seconds / 60;
  seconds = seconds % 60;

  return QString("%1:%2")
      .arg(minutes, 2, 10, QChar('0'))
      .arg(seconds, 2, 10, QChar('0'));
}

} // namespace ui
} // namespace cms
