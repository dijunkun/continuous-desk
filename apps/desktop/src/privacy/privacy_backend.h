/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-10
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_BACKEND_H_
#define _PRIVACY_BACKEND_H_

#include <cstdint>
#include <memory>
#include <string>

namespace crossdesk {

struct PrivacyScreenText {
  std::string verification = "Privacy screen verification";
  std::string unlock_hint = "To turn off the privacy screen, press";
};

struct PrivacyCapabilities {
  bool overlay = false;
  bool block_input = false;
  std::string reason;
};

struct PrivacyHealth {
  std::string failure;
  bool emergency_exit = false;
};

// Lifecycle, message pumping and native resource operations run on the
// controller's message thread. Const metadata queries are serialized with
// those operations by the controller mutex and must not call native UI APIs.
// No platform backend is installed on macOS/Linux yet.
class PrivacyBackend {
 public:
  virtual ~PrivacyBackend() = default;
  virtual PrivacyCapabilities Query() = 0;
  virtual bool Enable(bool block_input, const PrivacyScreenText& text,
                      std::string& error) = 0;
  virtual void Recover() = 0;
  virtual bool IsRecovered() const = 0;
  virtual PrivacyHealth Poll(bool force_check = false) = 0;
  // Wait on both native window messages and controller commands.
  // Wake is the only backend operation callable from another thread.
  virtual void WaitForEvents(unsigned timeout_ms) = 0;
  virtual void Wake() = 0;
  virtual bool PaintChallenge(uint32_t value, const std::string& title,
                              std::string& error) = 0;
  // Remove only the temporary capture probes, keeping all privacy covers up.
  // The controller drains queued probe frames before admitting clean video.
  virtual bool ClearChallenges(std::string& error) = 0;
  virtual bool HasMonitor(int left, int top, int width, int height) const = 0;
};

#ifdef _WIN32
std::unique_ptr<PrivacyBackend> CreateWindowsPrivacyBackend();
bool IsWindowsPrivacyDesktopAvailable();
#endif

}  // namespace crossdesk

#endif  // _PRIVACY_BACKEND_H_
