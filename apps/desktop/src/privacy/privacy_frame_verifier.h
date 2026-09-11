/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-10
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_FRAME_VERIFIER_H_
#define _PRIVACY_FRAME_VERIFIER_H_

#include <cstddef>
#include <cstdint>
#include <deque>

#include "privacy_probe_pattern.h"

namespace crossdesk {

// An opaque square code beside localized text, UNDER the privacy cover.
// Used only while transmission is gated during capture validation. Probes are
// then removed, and queued frames containing their tokens must be discarded.
class PrivacyFrameVerifier {
 public:
  void Reset(bool retain_history = false) {
    challenges_.clear();
    if (!retain_history) issued_values_.clear();
    matches_ = 0;
  }
  void Issue(uint32_t value, uint64_t now) {
    challenges_.push_back({value, now, false});
    issued_values_.push_back(value);
    // Covers more than the startup/drain deadlines, including backend retries.
    while (issued_values_.size() > 128) issued_values_.pop_front();
    // A validation attempt is time-bounded. Keep every issued token so an old
    // queued frame cannot escape merely because its token expired for matching.
  }
  bool Observe(const uint8_t* y, size_t size, int width, int height,
               uint64_t now) {
    uint32_t value = 0;
    if (!Decode(y, size, width, height, value)) return false;
    for (auto& challenge : challenges_) {
      if (challenge.value != value || now - challenge.issued > 750) continue;
      if (!challenge.seen) {
        challenge.seen = true;
        ++matches_;
      }
      return true;
    }
    return false;
  }
  bool Verified() const { return matches_ >= 2; }
  bool ContainsChallenge(const uint8_t* y, size_t size, int width,
                         int height) const {
    uint32_t value = 0;
    if (!Decode(y, size, width, height, value)) return false;
    for (const auto issued : issued_values_)
      if (issued == value) return true;
    return false;
  }

 private:
  static bool Decode(const uint8_t* y, size_t size, int width, int height,
                     uint32_t& value) {
    using Pattern = PrivacyProbePattern;
    if (!y || width < Pattern::kCaptureExtent ||
        height < Pattern::kCaptureExtent ||
        size < static_cast<size_t>(width) * height)
      return false;
    value = 0;
    for (int bit = 0; bit < 32; ++bit) {
      const int x = Pattern::kOffset + Pattern::kPadding +
                    (bit % Pattern::kColumns) * Pattern::kCell +
                    Pattern::kCell / 2;
      const int row = Pattern::kOffset + Pattern::kPadding +
                      (bit / Pattern::kColumns) * Pattern::kCell +
                      Pattern::kCell / 2;
      const auto pixel = y[static_cast<size_t>(row) * width + x];
      const auto inverse =
          y[static_cast<size_t>(row + 4 * Pattern::kCell) * width + x];
      if ((pixel > 60 && pixel < 190) || (inverse > 60 && inverse < 190) ||
          (pixel >= 190) == (inverse >= 190))
        return false;
      if (pixel >= 190) value |= uint32_t{1} << bit;
    }
    return true;
  }
  struct Challenge {
    uint32_t value;
    uint64_t issued;
    bool seen;
  };
  std::deque<Challenge> challenges_;
  std::deque<uint32_t> issued_values_;
  unsigned matches_ = 0;
};

}  // namespace crossdesk

#endif  // _PRIVACY_FRAME_VERIFIER_H_
