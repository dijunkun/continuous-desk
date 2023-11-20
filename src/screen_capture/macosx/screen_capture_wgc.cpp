#include "screen_capture_wgc.h"

#include <iostream>

ScreenCaptureWgc::ScreenCaptureWgc() {}

ScreenCaptureWgc::~ScreenCaptureWgc() {}

bool ScreenCaptureWgc::IsWgcSupported() { return false; }

int ScreenCaptureWgc::Init(const RECORD_DESKTOP_RECT &rect, const int fps,
                           cb_desktop_data cb) {
  return 0;
}

int ScreenCaptureWgc::Start() { return 0; }

int ScreenCaptureWgc::Pause() { return 0; }

int ScreenCaptureWgc::Resume() { return 0; }

int ScreenCaptureWgc::Stop() { return 0; }

void ScreenCaptureWgc::OnFrame() {}

void ScreenCaptureWgc::CleanUp() {}
