/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-11
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#include "privacy_probe_renderer.h"

#include <algorithm>

#include "privacy_probe_pattern.h"

namespace crossdesk {
namespace {
struct Canvas {
  HDC dc = CreateCompatibleDC(nullptr);
  HFONT font = nullptr;
  HGDIOBJ old_font = nullptr;
  HBITMAP bitmap = nullptr;
  HGDIOBJ old_bitmap = nullptr;
  ~Canvas() {
    if (old_bitmap) SelectObject(dc, old_bitmap);
    if (old_font) SelectObject(dc, old_font);
    if (bitmap) DeleteObject(bitmap);
    if (font) DeleteObject(font);
    if (dc) DeleteDC(dc);
  }
};
bool Failed(const char* operation, std::string& error) {
  error = std::string(operation) + " (Windows error " +
          std::to_string(GetLastError()) + ")";
  return false;
}
}  // namespace

bool RenderPrivacyProbe(const std::string& title, UINT dpi, int maximum_width,
                        int maximum_height, uint32_t value,
                        PrivacyProbeImage& image, std::string& error) {
  using Pattern = PrivacyProbePattern;
  const int length =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, title.data(),
                          static_cast<int>(title.size()), nullptr, 0);
  if (length <= 0) return Failed("Decode privacy verification text", error);
  std::wstring text(length, L'\0');
  if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, title.data(),
                           static_cast<int>(title.size()), text.data(), length))
    return Failed("Decode privacy verification text", error);
  const int available_text_width =
      maximum_width - Pattern::kTextLeft - Pattern::kPadding;
  if (available_text_width < 80 ||
      maximum_height < Pattern::kSide + 2 * Pattern::kPadding) {
    error = "Display is too small for the privacy verification panel";
    return false;
  }
  Canvas canvas;
  if (!canvas.dc)
    return Failed("Create privacy verification drawing context", error);
  canvas.font = CreateFontW(
      -MulDiv(28, static_cast<int>(dpi), 96), 0, 0, 0, FW_SEMIBOLD, FALSE,
      FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
  if (!canvas.font) return Failed("Create privacy verification font", error);
  canvas.old_font = SelectObject(canvas.dc, canvas.font);
  if (!canvas.old_font || canvas.old_font == HGDI_ERROR) {
    canvas.old_font = nullptr;
    return Failed("Select privacy verification font", error);
  }
  SIZE extent{};
  if (!GetTextExtentPoint32W(canvas.dc, text.data(), length, &extent))
    return Failed("Measure privacy verification text", error);
  const int text_width =
      (std::min)(available_text_width,
                 (std::min)(static_cast<int>(extent.cx) + 4,
                            MulDiv(520, static_cast<int>(dpi), 96)));
  RECT text_bounds{0, 0, text_width, 0};
  constexpr UINT text_flags = DT_LEFT | DT_WORDBREAK | DT_NOPREFIX;
  if (!DrawTextW(canvas.dc, text.data(), length, &text_bounds,
                 text_flags | DT_CALCRECT))
    return Failed("Lay out privacy verification text", error);
  const int width = Pattern::kTextLeft + text_width + Pattern::kPadding;
  const int height =
      (std::max)(Pattern::kSide, static_cast<int>(text_bounds.bottom)) +
      2 * Pattern::kPadding;
  if (height > maximum_height || text_bounds.right > text_width) {
    error =
        "Display is too small for localized privacy verification text at this "
        "DPI";
    return false;
  }
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = width;
  info.bmiHeader.biHeight = -height;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;
  void* bits = nullptr;
  canvas.bitmap =
      CreateDIBSection(canvas.dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
  if (!canvas.bitmap || !bits)
    return Failed("Create privacy verification image", error);
  canvas.old_bitmap = SelectObject(canvas.dc, canvas.bitmap);
  if (!canvas.old_bitmap || canvas.old_bitmap == HGDI_ERROR) {
    canvas.old_bitmap = nullptr;
    return Failed("Select privacy verification image", error);
  }
  auto pixels = static_cast<uint32_t*>(bits);
  std::fill(pixels, pixels + static_cast<size_t>(width) * height, 0xFFF4F6F9u);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      if (x == 0 || y == 0 || x == width - 1 || y == height - 1)
        pixels[static_cast<size_t>(y) * width + x] = 0xFFD0D6E0u;
  SetBkMode(canvas.dc, TRANSPARENT);
  SetTextColor(canvas.dc, RGB(34, 46, 64));
  RECT text_rect{Pattern::kTextLeft, (height - text_bounds.bottom) / 2,
                 width - Pattern::kPadding, (height + text_bounds.bottom) / 2};
  // Explicit clipping makes font fallback/overhang unable to reach the code.
  IntersectClipRect(canvas.dc, text_rect.left, text_rect.top, text_rect.right,
                    text_rect.bottom);
  if (!DrawTextW(canvas.dc, text.data(), length, &text_rect, text_flags))
    return Failed("Draw privacy verification text", error);
  if (!GdiFlush()) return Failed("Flush privacy verification text", error);
  for (size_t index = 0; index < static_cast<size_t>(width) * height; ++index)
    pixels[index] |= 0xFF000000u;
  for (int row = 0; row < Pattern::kRows; ++row)
    for (int column = 0; column < Pattern::kColumns; ++column) {
      const uint32_t color =
          Pattern::White(value, column, row) ? 0xFFFFFFFFu : 0xFF000000u;
      for (int y = 0; y < Pattern::kCell; ++y)
        for (int x = 0; x < Pattern::kCell; ++x)
          pixels[static_cast<size_t>(Pattern::kPadding + row * Pattern::kCell +
                                     y) *
                     width +
                 Pattern::kPadding + column * Pattern::kCell + x] = color;
    }
  image.width = width;
  image.height = height;
  image.pixels.assign(pixels, pixels + static_cast<size_t>(width) * height);
  return true;
}

}  // namespace crossdesk
