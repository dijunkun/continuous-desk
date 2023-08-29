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
  _rect = rect;
  _start_time = av_gettime_relative();
  _time_base = {1, AV_TIME_BASE};
  _pixel_fmt = AV_PIX_FMT_BGRA;
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
  std::cout << "onframe" << std::endl;
  AVFrame *av_frame = av_frame_alloc();

  av_frame->pts = av_gettime_relative();
  av_frame->pkt_dts = av_frame->pts;
  // av_frame->pkt_pts = av_frame->pts;

  av_frame->width = frame.width;
  av_frame->height = frame.height;
  av_frame->format = AV_PIX_FMT_BGRA;
  av_frame->pict_type = AV_PICTURE_TYPE_NONE;
  av_frame->pkt_size = frame.width * frame.height * 4;

  av_image_fill_arrays(av_frame->data, av_frame->linesize, frame.data,
                       AV_PIX_FMT_BGRA, frame.width, frame.height, 1);

  if (_on_data)
    _on_data((unsigned char *)av_frame->data, av_frame->pkt_size, frame.width,
             frame.height);

  // av_frame_free(&av_frame);

  // BGRA to YUV
  // auto swrCtxBGRA2YUV = sws_getContext(
  //     frame.width, frame.height, AV_PIX_FMT_BGRA, frame.width, frame.height,
  //     AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

  // create BGRA
  // AVFrame *frame_bgra = av_frame;
  // AVFrame *frame_bgra = av_frame_alloc();
  // frame_bgra->format = AV_PIX_FMT_BGRA;
  // frame_bgra->width = frame.width;
  // frame_bgra->height = frame.height;
  // if (av_frame_get_buffer(frame_bgra, 32) < 0) {
  //   printf("Failed: av_frame_get_buffer\n");
  //   return;
  // }
  // frame_bgra->data[0] = cropImage;

  // YUV
  // AVFrame *frame_yuv = av_frame_alloc();
  // frame_yuv->width = frame.width;
  // frame_yuv->height = frame.height;
  // frame_yuv->format = AV_PIX_FMT_YUV420P;

  // uint8_t *picture_buf =
  //     (uint8_t *)av_malloc(frame.width * frame.height * 3 / 2);
  // if (av_image_fill_arrays(frame_yuv->data, frame_yuv->linesize, picture_buf,
  //                          AV_PIX_FMT_YUV420P, frame.width, frame.height,
  //                          1) < 0) {
  //   std::cout << "Failed: av_image_fill_arrays" << std::endl;
  //   return;
  // }

  // if (sws_scale(swrCtxBGRA2YUV, frame_bgra->data, frame_bgra->linesize, 0,
  //               frame.height, frame_yuv->data, frame_yuv->linesize) < 0) {
  //   std::cout << "BGRA to YUV failed" << std::endl;
  //   return;
  // }

  // frame_yuv->pts = av_gettime();

  // if (_on_data)
  //   _on_data((unsigned char *)frame_yuv->data,
  //            frame.width * frame.height * 3 / 2, frame.width, frame.height);
}

void ScreenCaptureWgc::CleanUp() {
  _inited = false;

  if (session_) session_->Release();

  session_ = nullptr;
}
