/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-11
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_PROBE_RENDERER_H_
#define _PRIVACY_PROBE_RENDERER_H_

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

namespace crossdesk {

struct PrivacyProbeImage {
  int width = 0;
  int height = 0;
  std::vector<uint32_t> pixels;
};

// Produces an opaque BGRA panel. The localized text and the machine-readable
// square occupy disjoint rectangles. Caller runs on the owning window thread.
bool RenderPrivacyProbe(const std::string& title, UINT dpi, int maximum_width,
                        int maximum_height, uint32_t value,
                        PrivacyProbeImage& image, std::string& error);

}  // namespace crossdesk

#endif  // _PRIVACY_PROBE_RENDERER_H_
