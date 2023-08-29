#ifndef _SCREEN_CAPTURE_WGC_H_
#define _SCREEN_CAPTURE_WGC_H_

#include <Windows.h>

#include "wgc_session.h"
#include "wgc_session_impl.h"

extern "C" {
#include <libavcodec\avcodec.h>
#include <libavfilter\avfilter.h>
#include <libavformat\avformat.h>
#include <libavutil\imgutils.h>
#include <libavutil\time.h>
#include <libswscale/swscale.h>
}

#include <atomic>
#include <functional>
#include <string>
#include <thread>

typedef struct {
  int left;
  int top;
  int right;
  int bottom;
} RECORD_DESKTOP_RECT;

typedef std::function<void(unsigned char *, int, int, int)> cb_desktop_data;
typedef std::function<void(int)> cb_desktop_error;

class ScreenCaptureWgc : public WgcSession::wgc_session_observer {
 public:
  ScreenCaptureWgc();
  ~ScreenCaptureWgc();

 public:
  bool IsWgcSupported();

  int Init(const RECORD_DESKTOP_RECT &rect, const int fps, cb_desktop_data cb);

  int Start();
  int Pause();
  int Resume();
  int Stop();

  void OnFrame(const WgcSession::wgc_session_frame &frame);

 protected:
  void CleanUp();

 private:
  WgcSession *session_ = nullptr;

  std::atomic_bool _running;
  std::atomic_bool _paused;
  std::atomic_bool _inited;

  std::thread _thread;

  std::string _device_name;

  RECORD_DESKTOP_RECT _rect;

  int _fps;

  cb_desktop_data _on_data;
  cb_desktop_error _on_error;

  AVRational _time_base;
  int64_t _start_time;
  AVPixelFormat _pixel_fmt;
};

#endif