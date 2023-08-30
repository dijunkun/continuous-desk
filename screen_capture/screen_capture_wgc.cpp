#include "screen_capture_wgc.h"

#include <d3d11_4.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>

#include <iostream>

BOOL WINAPI EnumMonitorProc(HMONITOR hmonitor, HDC hdc, LPRECT lprc,
                            LPARAM data) {
  MONITORINFOEX info_ex;
  info_ex.cbSize = sizeof(MONITORINFOEX);

  GetMonitorInfo(hmonitor, &info_ex);

  if (info_ex.dwFlags == DISPLAY_DEVICE_MIRRORING_DRIVER) return true;

  if (info_ex.dwFlags & MONITORINFOF_PRIMARY) {
    *(HMONITOR *)data = hmonitor;
  }

  return true;
}

HMONITOR GetPrimaryMonitor() {
  HMONITOR hmonitor = nullptr;

  ::EnumDisplayMonitors(NULL, NULL, EnumMonitorProc, (LPARAM)&hmonitor);

  return hmonitor;
}

ScreenCaptureWgc::ScreenCaptureWgc() {}

ScreenCaptureWgc::~ScreenCaptureWgc() {
  Stop();
  CleanUp();
}

bool ScreenCaptureWgc::IsWgcSupported() {
  try {
    /* no contract for IGraphicsCaptureItemInterop, verify 10.0.18362.0 */
    return winrt::Windows::Foundation::Metadata::ApiInformation::
        IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 8);
  } catch (const winrt::hresult_error &) {
    return false;
  } catch (...) {
    return false;
  }
}

int ScreenCaptureWgc::Init(const RECORD_DESKTOP_RECT &rect, const int fps,
                           cb_desktop_data cb) {
  int error = 0;
  if (_inited == true) return error;

  _fps = fps;

  _on_data = cb;

  do {
    if (!IsWgcSupported()) {
      std::cout << "AE_UNSUPPORT" << std::endl;
      error = 2;
      break;
    }

    session_ = new WgcSessionImpl();
    if (!session_) {
      error = -1;
      std::cout << "AE_WGC_CREATE_CAPTURER_FAILED" << std::endl;
      break;
    }

    session_->RegisterObserver(this);

    error = session_->Initialize(GetPrimaryMonitor());

    _inited = true;
  } while (0);

  if (error != 0) {
  }

  return error;
}

int ScreenCaptureWgc::Start() {
  if (_running == true) {
    std::cout << "record desktop duplication is already running" << std::endl;
    return 0;
  }

  if (_inited == false) {
    std::cout << "AE_NEED_INIT" << std::endl;
    return 4;
  }

  _running = true;
  session_->Start();

  return 0;
}

int ScreenCaptureWgc::Pause() {
  _paused = true;
  if (session_) session_->Pause();
  return 0;
}

int ScreenCaptureWgc::Resume() {
  _paused = false;
  if (session_) session_->Resume();
  return 0;
}

int ScreenCaptureWgc::Stop() {
  _running = false;

  if (session_) session_->Stop();

  return 0;
}

void ScreenCaptureWgc::OnFrame(const WgcSession::wgc_session_frame &frame) {
  if (_on_data)
    _on_data((unsigned char *)frame.data, frame.width * frame.height * 4,
             frame.width, frame.height);
}

void ScreenCaptureWgc::CleanUp() {
  _inited = false;

  if (session_) session_->Release();

  session_ = nullptr;
}
