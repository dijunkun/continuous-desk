/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-11
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#include "privacy_cover_renderer.h"

#include <algorithm>
#include <array>

namespace crossdesk {
namespace {

constexpr size_t kInstructionLines = 2;
constexpr size_t kBrandLine = kInstructionLines;

struct PaintResources {
  HDC dc;
  int saved;
  HICON logo = nullptr;
  std::array<HFONT, kInstructionLines + 1> fonts{};
  ~PaintResources() {
    // Deselect every owned GDI object before releasing it.
    if (saved) RestoreDC(dc, saved);
    for (auto font : fonts)
      if (font) DeleteObject(font);
    if (logo) DestroyIcon(logo);
  }
};

bool Select(HDC dc, HGDIOBJ object) {
  const auto previous = SelectObject(dc, object);
  return previous && previous != HGDI_ERROR;
}

}  // namespace

bool DrawPrivacyCover(HDC dc, const RECT& rect, UINT dpi, HINSTANCE module,
                      const wchar_t* unlock_hint) {
  if (!FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH))))
    return false;
  const int width = rect.right - rect.left;
  const int height = rect.bottom - rect.top;
  if (width <= 0 || height <= 0) return false;
  // Scale with the monitor DPI, shrinking the whole group on small displays.
  // All coordinates remain in the owning window's physical-pixel context.
  const double scale = (std::min)({dpi / 96.0, width / 640.0, height / 200.0});
  const auto px = [scale](int value) {
    return (std::max)(1, static_cast<int>(value * scale + 0.5));
  };
  const int margin = px(32);
  const int text_width = px(398);
  PaintResources resources{dc, SaveDC(dc)};
  if (!resources.saved || !SetBkMode(dc, TRANSPARENT)) return false;

  struct Line {
    const wchar_t* text;
    int size;
    int weight;
    COLORREF color;
    RECT bounds{};
  };
  std::array<Line, kInstructionLines + 1> lines{{
      {unlock_hint, 16, FW_NORMAL, RGB(181, 190, 204)},
      {L"Ctrl + Alt + Shift + F12", 24, FW_SEMIBOLD, RGB(245, 247, 250)},
      {L"CrossDesk", 14, FW_SEMIBOLD, RGB(245, 247, 250)},
  }};
  constexpr UINT flags = DT_LEFT | DT_WORDBREAK | DT_NOPREFIX;
  for (size_t i = 0; i < lines.size(); ++i) {
    auto& line = lines[i];
    resources.fonts[i] =
        CreateFontW(-px(line.size), 0, 0, 0, line.weight, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
    if (!resources.fonts[i] || !Select(dc, resources.fonts[i])) return false;
    line.bounds = {0, 0, text_width, 0};
    if (!DrawTextW(dc, line.text, -1, &line.bounds, flags | DT_CALCRECT) ||
        line.bounds.right > text_width)
      return false;
  }
  const int content_height =
      static_cast<int>(lines[0].bounds.bottom + lines[1].bounds.bottom) +
      px(12);
  // Match the icon width to the name; each side keeps its own height.
  const int brand_gap = px(6);
  const int brand_width = static_cast<int>(lines[kBrandLine].bounds.right);
  const int logo_size = brand_width;
  const int brand_height =
      logo_size + brand_gap + static_cast<int>(lines[kBrandLine].bounds.bottom);
  if (logo_size <= 0 ||
      (std::max)(brand_height, content_height) + 2 * margin > height ||
      brand_width + px(24) + text_width + 2 * margin > width)
    return false;
  resources.logo = static_cast<HICON>(LoadImageW(
      module, L"IDI_CROSSDESK_LOGO", IMAGE_ICON, logo_size, logo_size, 0));
  if (!resources.logo) return false;
  const int brand_left = rect.left + margin;
  const int left = brand_left + brand_width + px(24);
  const int brand_top = rect.bottom - margin - brand_height;
  int top = rect.bottom - margin - content_height;
  if (!DrawIconEx(dc, brand_left, brand_top, resources.logo, logo_size,
                  logo_size, 0, nullptr, DI_NORMAL))
    return false;
  if (!Select(dc, resources.fonts[kBrandLine])) return false;
  RECT brand_bounds{brand_left, brand_top + logo_size + brand_gap,
                    brand_left + brand_width, rect.bottom - margin};
  SetTextColor(dc, lines[kBrandLine].color);
  if (!DrawTextW(dc, lines[kBrandLine].text, -1, &brand_bounds,
                 DT_CENTER | DT_SINGLELINE | DT_NOPREFIX))
    return false;
  for (size_t i = 0; i < kInstructionLines; ++i) {
    const auto& line = lines[i];
    if (!Select(dc, resources.fonts[i])) return false;
    RECT bounds{left, top, left + text_width, top + line.bounds.bottom};
    SetTextColor(dc, line.color);
    if (!DrawTextW(dc, line.text, -1, &bounds, flags)) return false;
    top += line.bounds.bottom + px(12);
  }
  return true;
}

}  // namespace crossdesk
