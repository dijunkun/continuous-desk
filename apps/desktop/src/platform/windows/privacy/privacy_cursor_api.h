/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-10
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_CURSOR_API_H_
#define _PRIVACY_CURSOR_API_H_

#include <Windows.h>

namespace crossdesk {

// A separate cursor plane can remain visible above a capture-excluded HWND.
// Magnification's documented cursor API hides that plane across input queues;
// it does not change cursor schemes, shapes, positions, focus or DPI awareness.
class PrivacyCursorApi {
 public:
  PrivacyCursorApi() = default;
  PrivacyCursorApi(const PrivacyCursorApi&) = delete;
  PrivacyCursorApi& operator=(const PrivacyCursorApi&) = delete;
  ~PrivacyCursorApi() { RestoreAndClose(); }
  bool Initialize() {
    module_ = LoadLibraryExW(L"Magnification.dll", nullptr,
                             LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module_) return false;
    const auto initialize = reinterpret_cast<InitializeFn>(
        GetProcAddress(module_, "MagInitialize"));
    uninitialize_ = reinterpret_cast<InitializeFn>(
        GetProcAddress(module_, "MagUninitialize"));
    show_ = reinterpret_cast<ShowFn>(
        GetProcAddress(module_, "MagShowSystemCursor"));
    if (!initialize || !uninitialize_ || !show_) {
      SetLastError(ERROR_PROC_NOT_FOUND);
      return false;
    }
    initialized_ = initialize() != FALSE;
    return initialized_;
  }
  bool Hide() { return initialized_ && show_(FALSE); }
  bool RestoreAndClose() {
    bool ok = true;
    if (initialized_) {
      ok = show_(TRUE) != FALSE;
      ok = (uninitialize_() != FALSE) && ok;
      initialized_ = false;
    }
    if (module_) {
      FreeLibrary(module_);
      module_ = nullptr;
    }
    return ok;
  }

 private:
  using InitializeFn = BOOL(WINAPI*)();
  using ShowFn = BOOL(WINAPI*)(BOOL);
  HMODULE module_ = nullptr;
  InitializeFn uninitialize_ = nullptr;
  ShowFn show_ = nullptr;
  bool initialized_ = false;
};

struct PrivacyCursorReady {
  volatile LONG error;
};

}  // namespace crossdesk

#endif  // _PRIVACY_CURSOR_API_H_
