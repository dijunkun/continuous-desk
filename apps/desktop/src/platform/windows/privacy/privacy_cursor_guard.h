/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-10
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_CURSOR_GUARD_H_
#define _PRIVACY_CURSOR_GUARD_H_

#include <Windows.h>

#include <string>

namespace crossdesk {

// Called only on the privacy message thread. The child owns cursor suppression
// and restores it when either this guard stops it or its parent process exits.
class PrivacyCursorGuard {
 public:
  PrivacyCursorGuard() = default;
  PrivacyCursorGuard(const PrivacyCursorGuard&) = delete;
  PrivacyCursorGuard& operator=(const PrivacyCursorGuard&) = delete;
  ~PrivacyCursorGuard();
  bool Start(std::string& error);
  bool Healthy(std::string& error);
  bool Stop(std::string& error);
  bool stopped() const { return !process_ && !restore_pending_; }

 private:
  bool RestoreAfterFailure(std::string& error);
  HANDLE process_ = nullptr;
  HANDLE stop_event_ = nullptr;
  bool restore_pending_ = false;
  bool helper_failure_reported_ = false;
};

}  // namespace crossdesk

#endif  // _PRIVACY_CURSOR_GUARD_H_
