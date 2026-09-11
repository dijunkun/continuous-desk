/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-11
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_PROBE_PATTERN_H_
#define _PRIVACY_PROBE_PATTERN_H_

#include <cstdint>

namespace crossdesk {

// Physical-pixel geometry shared by the renderer and raw-frame verifier.
// Text can scale/wrap independently without moving any sampling location.
struct PrivacyProbePattern {
  static constexpr int kOffset = 16;
  static constexpr int kPadding = 16;
  static constexpr int kGap = 24;
  static constexpr int kCell = 12;
  static constexpr int kColumns = 8;
  static constexpr int kRows = 8;
  static constexpr int kSide = kColumns * kCell;
  static constexpr int kCaptureExtent = kOffset + kPadding + kSide;
  static constexpr int kTextLeft = kPadding + kSide + kGap;

  // Top half carries all 32 bits; bottom half carries their complements.
  static constexpr bool White(uint32_t value, int column, int row) {
    const int index = row * kColumns + column;
    const bool bit = (value & (uint32_t{1} << (index % 32))) != 0;
    return index < 32 ? bit : !bit;
  }
};

}  // namespace crossdesk

#endif  // _PRIVACY_PROBE_PATTERN_H_
