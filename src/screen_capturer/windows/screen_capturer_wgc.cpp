#include "screen_capturer_wgc.h"

#include <Windows.h>
#include <d3d11_4.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};

#include <iostream>

int BGRAToNV12FFmpeg(unsigned char *src_buffer, int width, int height,
                     unsigned char *dst_buffer) {
  AVFrame *Input_pFrame = av_frame_alloc();
  AVFrame *Output_pFrame = av_frame_alloc();
  struct SwsContext *img_convert_ctx =
      sws_getContext(width, height, AV_PIX_FMT_BGRA, 1280, 720, AV_PIX_FMT_NV12,
                     SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  av_image_fill_arrays(Input_pFrame->data, Input_pFrame->linesize, src_buffer,
                       AV_PIX_FMT_BGRA, width, height, 1);
  av_image_fill_arrays(Output_pFrame->data, Output_pFrame->linesize, dst_buffer,
                       AV_PIX_FMT_NV12, 1280, 720, 1);

  sws_scale(img_convert_ctx, (uint8_t const **)Input_pFrame->data,
            Input_pFrame->linesize, 0, height, Output_pFrame->data,
            Output_pFrame->linesize);

  if (Input_pFrame) av_free(Input_pFrame);
  if (Output_pFrame) av_free(Output_pFrame);
  if (img_convert_ctx) sws_freeContext(img_convert_ctx);

  return 0;
}

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

ScreenCapturerWgc::ScreenCapturerWgc() {}

ScreenCapturerWgc::~ScreenCapturerWgc() {}

bool ScreenCapturerWgc::IsWgcSupported() {
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

int ScreenCapturerWgc::Init(const RECORD_DESKTOP_RECT &rect, const int fps,
                            cb_desktop_data cb) {
  int error = 0;
  if (_inited == true) return error;

  nv12_frame_ = new unsigned char[rect.right * rect.bottom * 4];

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

int ScreenCapturerWgc::Destroy() {
  if (nv12_frame_) {
    delete nv12_frame_;
    nv12_frame_ = nullptr;
  }

  Stop();
  CleanUp();

  return 0;
}

int ScreenCapturerWgc::Start() {
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

int ScreenCapturerWgc::Pause() {
  _paused = true;
  if (session_) session_->Pause();
  return 0;
}

int ScreenCapturerWgc::Resume() {
  _paused = false;
  if (session_) session_->Resume();
  return 0;
}

int ScreenCapturerWgc::Stop() {
  _running = false;

  if (session_) session_->Stop();

  return 0;
}

void ScreenCapturerWgc::OnFrame(const WgcSession::wgc_session_frame &frame) {
  if (_on_data)
    BGRAToNV12FFmpeg((unsigned char *)frame.data, frame.width, frame.height,
                     nv12_frame_);
  _on_data(nv12_frame_, frame.width * frame.height * 4, frame.width,
           frame.height);
}

void ScreenCapturerWgc::CleanUp() {
  _inited = false;

  if (session_) session_->Release();

  session_ = nullptr;
}
