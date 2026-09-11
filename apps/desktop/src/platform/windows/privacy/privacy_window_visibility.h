/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-11
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_WINDOW_VISIBILITY_H_
#define _PRIVACY_WINDOW_VISIBILITY_H_

#include <Windows.h>

#include <cstdint>

namespace crossdesk {

// Called for a visible foreign window whose full bounds intersect a cover.
// Inspect it on every check: an OSD helper can grow into a content window.
inline bool IsNonObstructingPrivacyWindow(HWND window, const RECT& bounds) {
  const auto styles = GetWindowLongPtrW(window, GWL_EXSTYLE);
  if (!(styles & WS_EX_LAYERED)) return false;

  BYTE alpha = 0;
  DWORD flags = 0;
  // WS_EX_TRANSPARENT alone does not imply invisible pixels. Likewise,
  // UpdateLayeredWindow windows can reject this query; failure proves nothing.
  if (GetLayeredWindowAttributes(window, nullptr, &alpha, &flags) &&
      (flags & LWA_ALPHA) && alpha == 0)
    return true;

  constexpr LONG_PTR helper_styles =
      WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT;
  const auto width = static_cast<int64_t>(bounds.right) - bounds.left;
  const auto height = static_cast<int64_t>(bounds.bottom) - bounds.top;
  // Single-pixel, nonactivating OSD placeholders cannot display meaningful
  // desktop content. Use the whole window, never its clipped intersection;
  // larger OSDs and windows with unknown opacity still require coverage checks.
  return (styles & helper_styles) == helper_styles && width == 1 && height == 1;
}

}  // namespace crossdesk

#endif  // _PRIVACY_WINDOW_VISIBILITY_H_
