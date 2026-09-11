/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-11
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SCREEN_CAPTURE_METHOD_H_
#define _SCREEN_CAPTURE_METHOD_H_

namespace crossdesk {

// Persisted values; keep these in the same order as the settings choices.
enum class ScreenCaptureMethod { Auto = 0, Dxgi = 1, Wgc = 2, Gdi = 3 };

inline bool IsValidScreenCaptureMethod(long value) {
  return value >= static_cast<long>(ScreenCaptureMethod::Auto) &&
         value <= static_cast<long>(ScreenCaptureMethod::Gdi);
}

}  // namespace crossdesk

#endif