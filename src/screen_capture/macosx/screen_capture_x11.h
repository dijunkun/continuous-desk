#ifndef _SCREEN_CAPTURE_X11_H_
#define _SCREEN_CAPTURE_X11_H_

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
};
#endif

typedef struct {
  int left;
  int top;
  int right;
  int bottom;
} RECORD_DESKTOP_RECT;

typedef std::function<void(unsigned char *, int, int, int)> cb_desktop_data;
typedef std::function<void(int)> cb_desktop_error;

class ScreenCaptureX11 {
 public:
  ScreenCaptureX11();
  ~ScreenCaptureX11();

 public:
  int Init(const RECORD_DESKTOP_RECT &rect, const int fps, cb_desktop_data cb);

  int Start();
  int Pause();
  int Resume();
  int Stop();

  void OnFrame();

 protected:
  void CleanUp();

 private:
  std::atomic_bool _running;
  std::atomic_bool _paused;
  std::atomic_bool _inited;

  std::thread _thread;

  std::string _device_name;

  RECORD_DESKTOP_RECT _rect;

  int _fps;

  cb_desktop_data _on_data;
  cb_desktop_error _on_error;

 private:
  int i_ = 0;
  int videoindex_ = 0;
  int got_picture_ = 0;
  // ffmpeg
  AVFormatContext *pFormatCtx_ = nullptr;
  AVCodecContext *pCodecCtx_ = nullptr;
  AVCodec *pCodec_ = nullptr;
  AVCodecParameters *pCodecParam_ = nullptr;
  AVDictionary *options_ = nullptr;
  AVInputFormat *ifmt_ = nullptr;
  AVFrame *pFrame_ = nullptr;
  AVFrame *pFrameNv12_ = nullptr;
  AVPacket *packet_ = nullptr;
  struct SwsContext *img_convert_ctx_ = nullptr;

  // thread
  std::unique_ptr<std::thread> capture_thread_ = nullptr;
};

#endif