#pragma once

#include <Windows.h>

#ifdef AMRECORDER_IMPORT
#define AMRECORDER_API extern "C" __declspec(dllimport)
#else
#define AMRECORDER_API extern "C" __declspec(dllexport)
#endif

namespace am {

class wgc_session {
public:
  struct wgc_session_frame {
    unsigned int width;
    unsigned int height;
    unsigned int row_pitch;

    const unsigned char *data;
  };

  class wgc_session_observer {
  public:
    virtual ~wgc_session_observer() {}
    virtual void on_frame(const wgc_session_frame &frame) = 0;
  };

public:
  virtual void release() = 0;

  virtual int initialize(HWND hwnd) = 0;
  virtual int initialize(HMONITOR hmonitor) = 0;

  virtual void register_observer(wgc_session_observer *observer) = 0;

  virtual int start() = 0;
  virtual int stop() = 0;

  virtual int pause() = 0;
  virtual int resume() = 0;

protected:
  virtual ~wgc_session(){};
};

} // namespace am

AMRECORDER_API bool wgc_is_supported();
AMRECORDER_API am::wgc_session *wgc_create_session();