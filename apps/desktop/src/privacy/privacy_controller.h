/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-10
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_CONTROLLER_H_
#define _PRIVACY_CONTROLLER_H_

#include <remote_action.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <random>
#include <thread>

#include "privacy_backend.h"
#include "privacy_frame_verifier.h"

namespace crossdesk {

class PrivacyController {
 public:
  using Factory = std::function<std::unique_ptr<PrivacyBackend>()>;
  explicit PrivacyController(Factory factory = {});
  ~PrivacyController();
  PrivacyController(const PrivacyController&) = delete;
  PrivacyController& operator=(const PrivacyController&) = delete;

  PrivacyStatus Snapshot() const;
  // The cover uses a snapshot of these strings when the next session starts.
  void SetText(const PrivacyScreenText& text);
  void Enable(bool block_input);
  // Arms protection synchronously before an incoming connection is published.
  // Unlike a manual request, startup/unavailable capture cannot leave the
  // newly connected peer with ordinary unprotected remote access.
  void EnableOnConnection();
  void Disable();
  // Called only after the last control session has been removed. Failures keep
  // protection while a session exists; ending that session recovers everything
  // and must not leave the next connection in a paused/failed state.
  void Disconnected();
  void Shutdown();
  void Fail(const std::string& reason);
  bool RemoteAllowed() const { return !remote_paused_.load(); }
  bool Engaged() const { return engaged_.load(); }
  void CaptureChanged(const std::string& backend, bool running);
  void CaptureInterrupted();
  void DisplayChanging();
  bool ObserveFrame(const uint8_t* y, size_t size, int width, int height,
                    int left, int top, int physical_width, int physical_height);

 private:
  enum class VerificationPhase { challenge, draining, live };
  void Run(Factory factory);
  void WakeLocked();
  void SetState(PrivacyState state, const std::string& reason);
  void FailLocked(const std::string& reason);
  void BeginVerification(const std::string& reason = {});
  static uint64_t Now();

  mutable std::mutex mutex_;
  std::condition_variable wake_;
  std::thread thread_;
  std::unique_ptr<PrivacyBackend> backend_;
  PrivacyStatus status_{};
  PrivacyFrameVerifier verifier_;
  PrivacyScreenText text_;
  std::mt19937 random_{std::random_device{}()};
  std::atomic<bool> remote_paused_{false};
  std::atomic<bool> engaged_{false};
  std::atomic<bool> drain_after_recovery_{false};
  VerificationPhase verification_phase_ = VerificationPhase::challenge;
  bool challenges_visible_ = false;
  unsigned clean_frames_ = 0;
  uint64_t last_frame_received_ = 0;
  bool quit_ = false;
  bool enable_pending_ = false;
  bool disable_pending_ = false;
  bool block_requested_ = false;
  bool capture_running_ = false;
  bool ever_enabled_ = false;
  bool automatic_enable_ = false;
  uint64_t capture_start_deadline_ = 0;
  std::string capture_backend_;
  uint64_t started_ = 0;
  uint64_t last_issue_ = 0;
  uint64_t last_capability_check_ = 0;
  int frame_left_ = 0;
  int frame_top_ = 0;
  bool frame_monitor_set_ = false;
  bool first_verified_frame_logged_ = false;
};

}  // namespace crossdesk

#endif  // _PRIVACY_CONTROLLER_H_
