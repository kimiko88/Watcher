#include "cms/ScreenRecorder.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

#ifdef CMS_HAS_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace cms {
namespace fs = std::filesystem;

// Internal implementation class (Pimpl pattern)
class ScreenRecorder::Impl {
public:
  std::string output_directory;
  std::string current_filename;
  Quality quality = Quality::MEDIUM;
  uint32_t frame_rate = 30;
  bool recording = false;
  bool paused = false;

  // Stats tracking
  uint64_t frame_count = 0;
  uint64_t duration_seconds = 0;
  float file_size_mb = 0.0f;
  uint32_t frame_drops = 0;

  mutable std::mutex mutex;
  std::thread encoding_thread;
  bool stop_requested = false;

#ifdef CMS_HAS_FFMPEG
  AVFormatContext *format_ctx = nullptr;
  AVCodecContext *codec_ctx = nullptr;
  AVFrame *frame = nullptr;
  AVPacket *packet = nullptr;
  SwsContext *sws_ctx = nullptr;
  AVStream *video_stream = nullptr;

  int width = 1920;
  int height = 1080;
#endif

  explicit Impl(const std::string &dir) : output_directory(dir) {}

  ~Impl() {
    if (recording) {
      stopRecording();
    }
    if (encoding_thread.joinable()) {
      encoding_thread.join();
    }
  }

  bool startRecording(const std::string &filename, Quality qual) {
    std::lock_guard<std::mutex> lock(mutex);

    if (recording) {
      return false; // Already recording
    }

    current_filename = filename;
    quality = qual;
    recording = true;
    paused = false;
    frame_count = 0;
    duration_seconds = 0;
    frame_drops = 0;
    stop_requested = false;

    // Create output directory if it doesn't exist
    fs::create_directories(output_directory);

    // Get resolution based on quality
    setResolutionFromQuality();

#ifdef CMS_HAS_FFMPEG
    // Initialize FFmpeg encoding
    if (!initializeFFmpeg()) {
      recording = false;
      return false;
    }
#endif

    // Start encoding thread
    encoding_thread = std::thread([this]() { encodingLoop(); });

    return true;
  }

  bool stopRecording() {
    {
      std::lock_guard<std::mutex> lock(mutex);

      if (!recording) {
        return false;
      }

      recording = false;
      paused = false;
      stop_requested = true;
    }

    // Wait for encoding thread to finish
    if (encoding_thread.joinable()) {
      encoding_thread.join();
    }

#ifdef CMS_HAS_FFMPEG
    cleanupFFmpeg();
#else
    // Create placeholder video file (for testing without FFmpeg)
    createPlaceholderVideo();
#endif

    return true;
  }

  bool pauseRecording() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!recording || paused) {
      return false;
    }

    paused = true;
    return true;
  }

  bool resumeRecording() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!recording || !paused) {
      return false;
    }

    paused = false;
    return true;
  }

  bool cancelRecording() {
    {
      std::lock_guard<std::mutex> lock(mutex);

      if (!recording) {
        return false;
      }

      recording = false;
      paused = false;
      stop_requested = true;
    }

    // Wait for thread
    if (encoding_thread.joinable()) {
      encoding_thread.join();
    }

#ifdef CMS_HAS_FFMPEG
    cleanupFFmpeg();
#endif

    // Delete partial file
    std::string path = getOutputPath();
    if (fs::exists(path)) {
      fs::remove(path);
    }

    return true;
  }

  RecordingStats getStats() const {
    std::lock_guard<std::mutex> lock(mutex);

    RecordingStats stats;
    stats.filename = current_filename;
    stats.duration_seconds = duration_seconds;
    stats.frame_count = frame_count;
    stats.file_size_mb = file_size_mb;
    stats.fps_actual = static_cast<float>(frame_rate);
    stats.bitrate_kbps = calculateBitrate();
    stats.codec = getCodecName();
    stats.resolution = getResolution();
    stats.disk_free_mb = getDiskFreeSpace();
    stats.frame_drops = frame_drops;

    return stats;
  }

  std::string getOutputPath() const {
    std::string extension = getFileExtension();
    fs::path path = fs::path(output_directory) / (current_filename + extension);
    return path.string();
  }

private:
  void setResolutionFromQuality() {
#ifdef CMS_HAS_FFMPEG
    switch (quality) {
    case Quality::LOW:
      width = 1280;
      height = 720;
      frame_rate = 15;
      break;
    case Quality::MEDIUM:
      width = 1920;
      height = 1080;
      frame_rate = 24;
      break;
    case Quality::HIGH:
      width = 2560;
      height = 1440;
      frame_rate = 30;
      break;
    case Quality::ULTRA:
      width = 3840;
      height = 2160;
      frame_rate = 60;
      break;
    }
#endif
  }

#ifdef CMS_HAS_FFMPEG
  bool initializeFFmpeg() {
    // Allocate format context
    std::string output_path = getOutputPath();
    avformat_alloc_output_context2(&format_ctx, nullptr, nullptr,
                                   output_path.c_str());
    if (!format_ctx) {
      return false;
    }

    // Find encoder
    const AVCodec *codec = nullptr;
    if (quality == Quality::LOW || quality == Quality::MEDIUM) {
      codec = avcodec_find_encoder_by_name("libvpx-vp9"); // VP9
      if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_VP9);
      }
    } else {
      codec = avcodec_find_encoder_by_name("libx265"); // H.265
      if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
      }
    }

    // Fallback to H.264 if others not available
    if (!codec) {
      codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }

    if (!codec) {
      avformat_free_context(format_ctx);
      format_ctx = nullptr;
      return false;
    }

    // Create video stream
    video_stream = avformat_new_stream(format_ctx, nullptr);
    if (!video_stream) {
      avformat_free_context(format_ctx);
      format_ctx = nullptr;
      return false;
    }

    // Allocate codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
      avformat_free_context(format_ctx);
      format_ctx = nullptr;
      return false;
    }

    // Set codec parameters
    codec_ctx->codec_id = codec->id;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base = AVRational{1, static_cast<int>(frame_rate)};
    codec_ctx->framerate = AVRational{static_cast<int>(frame_rate), 1};
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    // Set bitrate based on quality
    int bitrate = 2000000; // 2 Mbps default
    switch (quality) {
    case Quality::LOW:
      bitrate = 1000000;
      break; // 1 Mbps
    case Quality::MEDIUM:
      bitrate = 2500000;
      break; // 2.5 Mbps
    case Quality::HIGH:
      bitrate = 5000000;
      break; // 5 Mbps
    case Quality::ULTRA:
      bitrate = 10000000;
      break; // 10 Mbps
    }
    codec_ctx->bit_rate = bitrate;

    // Set GOP size
    codec_ctx->gop_size = frame_rate;
    codec_ctx->max_b_frames = 2;

    // Some formats require global headers
    if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
      codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // Open codec
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
      avcodec_free_context(&codec_ctx);
      avformat_free_context(format_ctx);
      codec_ctx = nullptr;
      format_ctx = nullptr;
      return false;
    }

    // Copy codec parameters to stream
    avcodec_parameters_from_context(video_stream->codecpar, codec_ctx);
    video_stream->time_base = codec_ctx->time_base;

    // Allocate frame and packet
    frame = av_frame_alloc();
    packet = av_packet_alloc();

    if (!frame || !packet) {
      cleanupFFmpeg();
      return false;
    }

    frame->format = codec_ctx->pix_fmt;
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;

    if (av_frame_get_buffer(frame, 0) < 0) {
      cleanupFFmpeg();
      return false;
    }

    // Open output file
    if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
      if (avio_open(&format_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE) <
          0) {
        cleanupFFmpeg();
        return false;
      }
    }

    // Write header
    if (avformat_write_header(format_ctx, nullptr) < 0) {
      cleanupFFmpeg();
      return false;
    }

    return true;
  }

  void cleanupFFmpeg() {
    // Write trailer
    if (format_ctx && video_stream) {
      av_write_trailer(format_ctx);
    }

    // Free resources
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
      if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&format_ctx->pb);
      }
      avformat_free_context(format_ctx);
      format_ctx = nullptr;
    }

    video_stream = nullptr;

    // Update file size
    std::string path = getOutputPath();
    if (fs::exists(path)) {
      file_size_mb =
          static_cast<float>(fs::file_size(path)) / (1024.0f * 1024.0f);
    }
  }
#endif

  void encodingLoop() {
#ifdef CMS_HAS_FFMPEG
    auto start_time = std::chrono::steady_clock::now();
    int64_t pts = 0;

    while (!stop_requested) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        if (!recording)
          break;

        if (paused) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }
      }

      // Capture screen frame
      if (!captureScreenFrame()) {
        frame_drops++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }

      // Encode frame
      frame->pts = pts++;

      if (avcodec_send_frame(codec_ctx, frame) == 0) {
        while (avcodec_receive_packet(codec_ctx, packet) == 0) {
          av_packet_rescale_ts(packet, codec_ctx->time_base,
                               video_stream->time_base);
          packet->stream_index = video_stream->index;

          av_interleaved_write_frame(format_ctx, packet);
          av_packet_unref(packet);
        }
      }

      frame_count++;

      auto now = std::chrono::steady_clock::now();
      duration_seconds =
          std::chrono::duration_cast<std::chrono::seconds>(now - start_time)
              .count();

      // Frame rate limiting
      std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frame_rate));
    }
#else
    // Stub encoding loop without FFmpeg
    auto start_time = std::chrono::steady_clock::now();

    while (!stop_requested) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        if (!recording)
          break;

        if (!paused) {
          frame_count++;

          auto now = std::chrono::steady_clock::now();
          duration_seconds =
              std::chrono::duration_cast<std::chrono::seconds>(now - start_time)
                  .count();
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frame_rate));
    }
#endif
  }

#ifdef CMS_HAS_FFMPEG
  bool captureScreenFrame() {
#ifdef _WIN32
    // Windows screen capture using GDI
    HDC screen_dc = GetDC(nullptr);
    HDC mem_dc = CreateCompatibleDC(screen_dc);

    // Get screen dimensions
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP bitmap =
        CreateCompatibleBitmap(screen_dc, screen_width, screen_height);
    HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, bitmap);

    // Copy screen to bitmap
    BitBlt(mem_dc, 0, 0, screen_width, screen_height, screen_dc, 0, 0, SRCCOPY);

    // Get bitmap data
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = screen_width;
    bi.biHeight = -screen_height; // Top-down
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    int data_size = screen_width * screen_height * 3;
    std::vector<uint8_t> buffer(data_size);

    GetDIBits(mem_dc, bitmap, 0, screen_height, buffer.data(),
              (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    // Convert BGR to YUV420P using swscale
    if (!sws_ctx) {
      sws_ctx = sws_getContext(screen_width, screen_height, AV_PIX_FMT_BGR24,
                               width, height, AV_PIX_FMT_YUV420P, SWS_BILINEAR,
                               nullptr, nullptr, nullptr);
    }

    if (sws_ctx) {
      // Create temporary frame for RGB data
      uint8_t *src_data[1] = {buffer.data()};
      int src_linesize[1] = {screen_width * 3};

      sws_scale(sws_ctx, src_data, src_linesize, 0, screen_height, frame->data,
                frame->linesize);
    }

    // Cleanup
    SelectObject(mem_dc, old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(mem_dc);
    ReleaseDC(nullptr, screen_dc);

    return sws_ctx != nullptr;
#else
    // Platform not supported for real screen capture
    // Fill frame with test pattern
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        frame->data[0][y * frame->linesize[0] + x] =
            (x + y + frame_count) % 256;
      }
    }
    return true;
#endif
  }
#endif

  void createPlaceholderVideo() {
    // Create a placeholder video file for testing without FFmpeg
    std::string path = getOutputPath();
    std::ofstream file(path, std::ios::binary);

    if (file.is_open()) {
      // Write minimal header
      file << "PLACEHOLDER_VIDEO_DATA";
      file.close();

      file_size_mb =
          static_cast<float>(fs::file_size(path)) / (1024.0f * 1024.0f);
    }
  }

  std::string getFileExtension() const {
    switch (quality) {
    case Quality::LOW:
    case Quality::MEDIUM:
      return ".webm"; // VP9
    case Quality::HIGH:
    case Quality::ULTRA:
      return ".mp4"; // H.265
    default:
      return ".mp4";
    }
  }

  std::string getCodecName() const {
    switch (quality) {
    case Quality::LOW:
    case Quality::MEDIUM:
      return "vp9";
    case Quality::HIGH:
    case Quality::ULTRA:
      return "hevc"; // H.265
    default:
      return "unknown";
    }
  }

  std::string getResolution() const {
#ifdef CMS_HAS_FFMPEG
    return std::to_string(width) + "x" + std::to_string(height);
#else
    switch (quality) {
    case Quality::LOW:
      return "1280x720";
    case Quality::MEDIUM:
      return "1920x1080";
    case Quality::HIGH:
      return "2560x1440";
    case Quality::ULTRA:
      return "3840x2160";
    default:
      return "0x0";
    }
#endif
  }

  float calculateBitrate() const {
    if (duration_seconds == 0)
      return 0.0f;
    return (file_size_mb * 8 * 1024) / duration_seconds; // kbps
  }

  float getDiskFreeSpace() const {
    try {
      auto space = fs::space(output_directory);
      return static_cast<float>(space.available) / (1024.0f * 1024.0f);
    } catch (...) {
      return 0.0f;
    }
  }
};

// Public interface implementation
ScreenRecorder::ScreenRecorder(const std::string &output_directory)
    : pImpl_(new Impl(output_directory)) {}

ScreenRecorder::~ScreenRecorder() { delete pImpl_; }

bool ScreenRecorder::startRecording(const std::string &filename,
                                    Quality quality) {
  return pImpl_->startRecording(filename, quality);
}

bool ScreenRecorder::stopRecording() { return pImpl_->stopRecording(); }

bool ScreenRecorder::isRecording() const {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);
  return pImpl_->recording;
}

bool ScreenRecorder::pauseRecording() { return pImpl_->pauseRecording(); }

bool ScreenRecorder::resumeRecording() { return pImpl_->resumeRecording(); }

bool ScreenRecorder::setFrameRate(uint32_t fps) {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);

  if (pImpl_->recording) {
    return false; // Cannot change during recording
  }

  if (fps < 1 || fps > 120) {
    return false; // Invalid range
  }

  pImpl_->frame_rate = fps;
  return true;
}

bool ScreenRecorder::setQuality(Quality quality) {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);

  if (pImpl_->recording) {
    return false; // Cannot change during recording
  }

  pImpl_->quality = quality;
  return true;
}

RecordingStats ScreenRecorder::getRecordingStats() const {
  return pImpl_->getStats();
}

std::string ScreenRecorder::getOutputPath() const {
  std::lock_guard<std::mutex> lock(pImpl_->mutex);
  return pImpl_->getOutputPath();
}

bool ScreenRecorder::cancelRecording() { return pImpl_->cancelRecording(); }

} // namespace cms
