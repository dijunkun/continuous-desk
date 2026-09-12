/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-13
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_CURSOR_MAC_H_
#define _PRIVACY_CURSOR_MAC_H_

#include <memory>
#include <cstdint>

namespace crossdesk {
// Main-thread resource. Hides only for the lifetime of a local privacy cover;
// does not take focus, move the pointer, or suppress remote input.
class MacPrivacyCursor {
 public:
  MacPrivacyCursor();
  ~MacPrivacyCursor();
  static bool Supported();
  bool Prepare();
  bool Hide();
  bool Healthy() const;
  uint32_t WindowId() const;
  void Restore();
 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Local hiding must not be reported as an invisible cursor to controllers.
bool IsMacPrivacyCursorHidden();
}

#endif