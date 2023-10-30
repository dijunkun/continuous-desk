#include "x11_session_impl.h"

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>

#define CHECK_INIT                            \
  if (!is_initialized_) {                     \
    std::cout << "AE_NEED_INIT" << std::endl; \
    return 4;                                 \
  }

X11SessionImpl::X11SessionImpl() {}

X11SessionImpl::~X11SessionImpl() {
  Stop();
  CleanUp();
}

void X11SessionImpl::Release() { delete this; }

int X11SessionImpl::Initialize() {}

void X11SessionImpl::RegisterObserver(x11_session_observer *observer) {
  observer_ = observer;
}

int X11SessionImpl::Start() {
  if (is_running_) return 0;

  int error = 1;

  CHECK_INIT;

  return error;
}

int X11SessionImpl::Stop() { return 0; }

int X11SessionImpl::Pause() {}

int X11SessionImpl::Resume() {}

void X11SessionImpl::OnFrame() {}

void X11SessionImpl::OnClosed() {}

int X11SessionImpl::Initialize() { return 0; }

void X11SessionImpl::CleanUp() {}