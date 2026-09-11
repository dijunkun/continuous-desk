/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-10
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _WINDOWS_THREAD_DPI_H_
#define _WINDOWS_THREAD_DPI_H_

#include <Windows.h>

namespace crossdesk {

// Keep newer DPI entry points out of the import table: privacy capability
// checks must still be able to report unsupported on pre-2004 Windows.
class ScopedWindowsPhysicalCoordinates {
 public:
  using SetContext = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
  ScopedWindowsPhysicalCoordinates() {
    setter_ = reinterpret_cast<SetContext>(GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext"));
    if (setter_) {
      previous_ = setter_(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
      // Ordinary remote input retains compatibility with Windows 10 1607.
      // The privacy backend itself requires build 19041 before using this.
      if (!previous_)
        previous_ = setter_(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }
  }
  ~ScopedWindowsPhysicalCoordinates() {
    if (setter_ && previous_) setter_(previous_);
  }
  ScopedWindowsPhysicalCoordinates(const ScopedWindowsPhysicalCoordinates&) =
      delete;
  ScopedWindowsPhysicalCoordinates& operator=(
      const ScopedWindowsPhysicalCoordinates&) = delete;
  bool active() const { return previous_ != nullptr; }

 private:
  SetContext setter_ = nullptr;
  DPI_AWARENESS_CONTEXT previous_ = nullptr;
};

inline UINT PrivacyWindowDpi(HWND hwnd) {
  using GetDpi = UINT(WINAPI*)(HWND);
  static const auto get_dpi = reinterpret_cast<GetDpi>(
      GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
  return get_dpi ? get_dpi(hwnd) : 96;
}

}  // namespace crossdesk

#endif  // _WINDOWS_THREAD_DPI_H_
