#ifndef _WGC_SESSION_IMPL_H_
#define _WGC_SESSION_IMPL_H_

#include <mutex>
#include <thread>

#include "x11_session.h"

class X11SessionImpl : public X11Session {
 public:
  X11SessionImpl();
  ~X11SessionImpl() override;

 public:
  void Release() override;

  int Initialize() override;

  void RegisterObserver(x11_session_observer *observer) override;

  int Start() override;
  int Stop() override;

  int Pause() override;
  int Resume() override;

 private:
  void OnFrame();
  void OnClosed();

  void CleanUp();

  // void message_func();

 private:
  std::mutex lock_;
  bool is_initialized_ = false;
  bool is_running_ = false;
  bool is_paused_ = false;

  x11_session_observer *observer_ = nullptr;
};

#endif