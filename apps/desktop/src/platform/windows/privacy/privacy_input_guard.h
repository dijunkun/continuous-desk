/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-10
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_INPUT_GUARD_H_
#define _PRIVACY_INPUT_GUARD_H_

#include <Windows.h>

#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace crossdesk {

// The owner controls lifecycle; the dedicated message thread owns LL hooks.
// Rendering and session queries never run on this thread.
class PrivacyInputGuard {
 public:
  ~PrivacyInputGuard();
  bool Start(HWND owner, UINT exit_message, std::string& error);
  void Stop();
  bool Healthy(std::string& error) const;
  bool stopped() const { return !thread_.joinable(); }

 private:
  void Run();
  bool ReleasePressedInputs();
  bool PumpMessages();
  void Fault(const std::string& operation, DWORD code);
  bool Renew(HHOOK& hook, int kind, HOOKPROC procedure);
  static LRESULT CALLBACK KeyboardHook(int code, WPARAM wp, LPARAM lp);
  static LRESULT CALLBACK MouseHook(int code, WPARAM wp, LPARAM lp);

  std::thread thread_;
  HANDLE stop_ = nullptr;
  HANDLE ready_ = nullptr;
  HWND owner_ = nullptr;
  UINT exit_message_ = 0;
  std::atomic<bool> healthy_{false};
  std::atomic<bool> running_{false};
  std::atomic<ULONGLONG> last_pump_{0};
  mutable std::mutex error_mutex_;
  std::string error_;
  HHOOK keyboard_ = nullptr;
  HHOOK mouse_ = nullptr;
  std::vector<HHOOK> retired_;
  std::array<bool, 256> physical_keys_{};
  inline static thread_local PrivacyInputGuard* current_ = nullptr;
};

}  // namespace crossdesk

#endif  // _PRIVACY_INPUT_GUARD_H_
