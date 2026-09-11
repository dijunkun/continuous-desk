/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-11
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#include "privacy_controller.h"

#include <chrono>
#include <cstring>

#include "rd_log.h"

namespace crossdesk {

uint64_t PrivacyController::Now() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

PrivacyController::PrivacyController(Factory factory) {
  status_.state = PrivacyState::unsupported;
  std::strcpy(status_.reason,
              "Privacy screen is not available on this platform");
#ifdef _WIN32
  if (!factory) factory = CreateWindowsPrivacyBackend;
#endif
  thread_ = std::thread([this, factory] { Run(factory); });
}

PrivacyController::~PrivacyController() { Shutdown(); }

void PrivacyController::WakeLocked() {
  if (backend_) backend_->Wake();
  wake_.notify_all();
}

void PrivacyController::Shutdown() {
  {
    std::lock_guard lock(mutex_);
    quit_ = true;
    remote_paused_.store(true);
    WakeLocked();
  }
  if (thread_.joinable()) thread_.join();
}

PrivacyStatus PrivacyController::Snapshot() const {
  std::lock_guard lock(mutex_);
  return status_;
}

void PrivacyController::SetText(const PrivacyScreenText& text) {
  std::lock_guard lock(mutex_);
  text_ = text;
}

void PrivacyController::SetState(PrivacyState state,
                                 const std::string& reason) {
  status_.state = state;
  status_.remote_paused = remote_paused_.load();
  ++status_.revision;
  std::strncpy(status_.reason, reason.c_str(), sizeof(status_.reason) - 1);
  status_.reason[sizeof(status_.reason) - 1] = '\0';
  LOG_INFO(
      "Privacy: state={}, overlay={}, input_blocked={}, paused={}, reason={}",
      static_cast<int>(state), status_.overlay_active, status_.input_blocked,
      status_.remote_paused, reason);
}

void PrivacyController::Enable(bool block_input) {
  std::lock_guard lock(mutex_);
  if (quit_) return;
  if (status_.state == PrivacyState::on ||
      status_.state == PrivacyState::starting ||
      status_.state == PrivacyState::stopping ||
      status_.state == PrivacyState::failed) {
    SetState(status_.state, status_.reason);
    return;
  }
  if (!status_.supported || (block_input && !status_.input_block_supported)) {
    SetState(PrivacyState::unsupported, status_.reason);
    return;
  }
  // Failure recovery is explicit: first turn off, then request a fresh enable.
  engaged_.store(true);
  remote_paused_.store(true);
  block_requested_ = block_input;
  automatic_enable_ = false;
  enable_pending_ = true;
  ever_enabled_ = false;
  SetState(PrivacyState::starting,
           "Creating privacy windows and checking capture");
  WakeLocked();
}

void PrivacyController::EnableOnConnection() {
  std::lock_guard lock(mutex_);
  if (quit_ || status_.state == PrivacyState::on ||
      status_.state == PrivacyState::starting ||
      status_.state == PrivacyState::failed)
    return;
  // Also accept a reconnect while the previous disconnect is still cleaning
  // up. Run() keeps the gate closed between recovery and this new enable.
  engaged_.store(true);
  remote_paused_.store(true);
  block_requested_ = true;
  automatic_enable_ = true;
  capture_start_deadline_ = Now() + 10000;
  enable_pending_ = true;
  ever_enabled_ = false;
  SetState(PrivacyState::starting,
           "Connected; automatically enabling privacy and verifying capture");
  WakeLocked();
}

void PrivacyController::Disable() {
  std::lock_guard lock(mutex_);
  if (quit_) return;
  if (!engaged_.load() && !status_.overlay_active) {
    SetState(status_.state, status_.reason);
    return;
  }
  enable_pending_ = false;
  disable_pending_ = true;
  remote_paused_.store(true);
  SetState(PrivacyState::stopping, "Restoring local desktop and input");
  WakeLocked();
}

void PrivacyController::Disconnected() {
  std::lock_guard lock(mutex_);
  if (quit_ ||
      (!engaged_.load() && !status_.overlay_active && !remote_paused_.load()))
    return;
  remote_paused_.store(true);
  enable_pending_ = false;
  disable_pending_ = true;
  SetState(PrivacyState::stopping,
           "Last controller disconnected; restoring desktop, input and session "
           "state");
  WakeLocked();
}

void PrivacyController::FailLocked(const std::string& reason) {
  if (!engaged_.load() || status_.state == PrivacyState::failed ||
      status_.state == PrivacyState::stopping)
    return;
  remote_paused_.store(true);
  enable_pending_ = false;
  SetState(PrivacyState::failed, reason);
}

void PrivacyController::Fail(const std::string& reason) {
  std::lock_guard lock(mutex_);
  FailLocked(reason);
}

void PrivacyController::BeginVerification(const std::string& reason) {
  remote_paused_.store(true);
  verifier_.Reset(true);
  verification_phase_ = VerificationPhase::challenge;
  clean_frames_ = 0;
  last_frame_received_ = 0;
  drain_after_recovery_.store(false);
  started_ = Now();
  last_issue_ = 0;
  frame_monitor_set_ = false;
  first_verified_frame_logged_ = false;
  SetState(PrivacyState::starting,
           !reason.empty() ? reason
           : automatic_enable_ && !capture_running_
               ? "Waiting for desktop capture startup"
               : "Verifying live desktop through " + capture_backend_);
}

void PrivacyController::CaptureChanged(const std::string& backend,
                                       bool running) {
  std::lock_guard lock(mutex_);
  capture_backend_ = backend;
  capture_running_ = running;
  if (!engaged_.load()) return;
  if (ever_enabled_) {
    FailLocked(
        "Capture backend stopped or rebuilt; remote operation paused. Turn "
        "privacy off to recover.");
  } else if (status_.state == PrivacyState::starting) {
    BeginVerification();
  }
}

void PrivacyController::DisplayChanging() {
  std::lock_guard lock(mutex_);
  if (status_.state == PrivacyState::on) BeginVerification();
}

void PrivacyController::CaptureInterrupted() {
  std::lock_guard lock(mutex_);
  if (!engaged_.load()) return;
  if (ever_enabled_) {
    FailLocked(
        "Capture component is rebuilding; remote operation paused. Turn "
        "privacy off to recover.");
  } else if (status_.state == PrivacyState::starting) {
    // The capturer owns recovery and backend selection. Require fresh frames
    // from it without letting repeated resets extend verification forever.
    const auto attempt_started = started_;
    BeginVerification(
        "Capture component interrupted; verifying recovered frames");
    if (attempt_started) started_ = attempt_started;
  }
}

bool PrivacyController::ObserveFrame(const uint8_t* y, size_t size, int width,
                                     int height, int left, int top,
                                     int physical_width, int physical_height) {
  if (!engaged_.load() && !drain_after_recovery_.load()) return RemoteAllowed();
  std::lock_guard lock(mutex_);
  if (!engaged_.load()) {
    // Disabling during verification can leave already-captured probe frames
    // queued behind HWND destruction. Do not leak them after reporting Off.
    if (drain_after_recovery_.load()) {
      if (!y || width <= 0 || height <= 0 ||
          size < static_cast<size_t>(width) * height)
        return false;
      if (verifier_.ContainsChallenge(y, size, width, height)) {
        clean_frames_ = 0;
        return false;
      }
      if (++clean_frames_ < 2) return false;
      drain_after_recovery_.store(false);
      verifier_.Reset();
    }
    return RemoteAllowed();
  }
  if (!backend_ || !status_.overlay_active ||
      (status_.state != PrivacyState::starting &&
       status_.state != PrivacyState::on))
    return false;
  if (!backend_->HasMonitor(left, top, physical_width, physical_height) ||
      width != (physical_width & ~1) || height != (physical_height & ~1)) {
    if (ever_enabled_)
      FailLocked(
          "Capture geometry changed or cannot be verified; remote operation "
          "paused");
    return false;
  }
  if (frame_monitor_set_ && (frame_left_ != left || frame_top_ != top)) {
    BeginVerification();
  }
  frame_left_ = left;
  frame_top_ = top;
  frame_monitor_set_ = true;
  if (!y || size < static_cast<size_t>(width) * height) {
    FailLocked(
        "Capture returned an invalid desktop frame; remote operation paused");
    return false;
  }
  last_frame_received_ = Now();
  if (verification_phase_ == VerificationPhase::challenge) {
    verifier_.Observe(y, size, width, height, last_frame_received_);
    return false;  // Verification pixels are never sent to the encoder.
  }
  if (verifier_.ContainsChallenge(y, size, width, height)) {
    clean_frames_ = 0;
    if (verification_phase_ == VerificationPhase::live) {
      drain_after_recovery_.store(true);
      FailLocked(
          "Capture reintroduced verification pixels; remote operation paused");
    }
    return false;
  }
  if (verification_phase_ == VerificationPhase::draining) {
    if (clean_frames_ < 2) ++clean_frames_;
    return false;  // Only the owner thread can publish the verified On state.
  }
  if (status_.state != PrivacyState::on) return false;
  if (!first_verified_frame_logged_) {
    first_verified_frame_logged_ = true;
    LOG_INFO(
        "Privacy: first clean verified desktop frame admitted, backend={}, "
        "size={}x{}, origin=({}, {})",
        capture_backend_, width, height, left, top);
  }
  return true;
}

void PrivacyController::Run(Factory factory) {
  std::unique_lock lock(mutex_);
  try {
    if (factory) backend_ = factory();
    if (backend_) {
      const auto caps = backend_->Query();
      last_capability_check_ = Now();
      status_.supported = caps.overlay;
      status_.input_block_supported = caps.block_input;
      // A connection can arm the gate before this thread has initialized.
      if (!engaged_.load())
        SetState(caps.overlay ? PrivacyState::off : PrivacyState::unsupported,
                 caps.reason);
    }
    while (!quit_) {
      if (backend_ && !engaged_.load() &&
          Now() - last_capability_check_ >= 2000) {
        last_capability_check_ = Now();
        const auto caps = backend_->Query();
        if (status_.supported != caps.overlay ||
            status_.input_block_supported != caps.block_input) {
          status_.supported = caps.overlay;
          status_.input_block_supported = caps.block_input;
          SetState(caps.overlay ? PrivacyState::off : PrivacyState::unsupported,
                   caps.reason);
        }
      }
      if (disable_pending_) {
        if (backend_) backend_->Recover();
        disable_pending_ = false;
        if (backend_ && !backend_->IsRecovered()) {
          enable_pending_ = false;
          SetState(PrivacyState::failed,
                   "Could not release every privacy resource; remote operation "
                   "remains paused");
          continue;
        }
        status_.overlay_active = status_.input_blocked = false;
        const bool drain = challenges_visible_ ||
                           verification_phase_ == VerificationPhase::draining ||
                           drain_after_recovery_.load();
        challenges_visible_ = false;
        clean_frames_ = 0;
        drain_after_recovery_.store(drain);
        if (!drain) verifier_.Reset();
        verification_phase_ = VerificationPhase::live;
        if (enable_pending_) {
          SetState(PrivacyState::starting,
                   "Reconnected; automatically restoring privacy protection");
        } else {
          ever_enabled_ = false;
          automatic_enable_ = false;
          block_requested_ = false;
          capture_start_deadline_ = 0;
          started_ = last_issue_ = 0;
          frame_monitor_set_ = false;
          first_verified_frame_logged_ = false;
          engaged_.store(false);
          remote_paused_.store(false);
          SetState(PrivacyState::off, "Privacy screen is off");
        }
      }
      if (enable_pending_) {
        enable_pending_ = false;
        std::string error;
        if (automatic_enable_) {
          const auto caps =
              backend_ ? backend_->Query()
                       : PrivacyCapabilities{false, false,
                                             "No privacy backend is installed"};
          status_.supported = caps.overlay;
          status_.input_block_supported = caps.block_input;
          if (!caps.overlay || !caps.block_input)
            FailLocked("Automatic privacy unavailable: " + caps.reason);
        }
        if (status_.state != PrivacyState::failed) {
          if (!capture_running_ && !automatic_enable_) {
            FailLocked("No running desktop capture; remote operation paused");
          } else if (!backend_->Enable(block_requested_, text_, error)) {
            backend_->Recover();
            status_.overlay_active = !backend_->IsRecovered();
            FailLocked(error);
          } else {
            status_.overlay_active = true;
            status_.input_blocked = block_requested_;
            BeginVerification();
          }
        }
      }
      // Pump the native queue even after recovery: the message sink is reused.
      const auto health =
          backend_ ? backend_->Poll(status_.state == PrivacyState::starting &&
                                    verifier_.Verified())
                   : PrivacyHealth{};
      if (backend_ && engaged_.load()) {
        if (health.emergency_exit) {
          remote_paused_.store(true);
          enable_pending_ = false;
          disable_pending_ = true;
          SetState(PrivacyState::stopping,
                   "Local privacy exit; restoring desktop and input before "
                   "resuming remote operation");
          continue;
        } else if (!health.failure.empty()) {
          FailLocked(health.failure);
        }
        if (status_.overlay_active &&
            (status_.state == PrivacyState::starting ||
             status_.state == PrivacyState::on)) {
          const auto now = Now();
          if (verification_phase_ == VerificationPhase::challenge &&
              now - last_issue_ >= 250) {
            uint32_t challenge;
            do {
              challenge = random_();
            } while (challenge == 0 || challenge == UINT32_MAX);
            std::string error;
            challenges_visible_ = true;
            // Record even a partially painted multi-monitor challenge, so
            // cleanup can reject its queued frames if another monitor fails.
            verifier_.Issue(challenge, now);
            if (!backend_->PaintChallenge(challenge, text_.verification,
                                          error)) {
              FailLocked(error);
            } else {
              last_issue_ = now;
            }
          }
          if (status_.state == PrivacyState::starting &&
              verification_phase_ == VerificationPhase::challenge &&
              verifier_.Verified() && capture_running_) {
            std::string error;
            if (!backend_->ClearChallenges(error)) {
              FailLocked(error);
            } else {
              challenges_visible_ = false;
              verification_phase_ = VerificationPhase::draining;
              clean_frames_ = 0;
              started_ = now;
              SetState(PrivacyState::starting,
                       "Capture exclusion verified; waiting for clean desktop "
                       "frames");
            }
          }
          if (status_.state == PrivacyState::starting &&
              verification_phase_ == VerificationPhase::draining &&
              clean_frames_ >= 2 && capture_running_) {
            verification_phase_ = VerificationPhase::live;
            ever_enabled_ = true;
            automatic_enable_ = false;
            remote_paused_.store(false);
            SetState(PrivacyState::on, "Privacy screen verified using " +
                                           capture_backend_ +
                                           "; verification windows removed");
          }
          if (status_.state == PrivacyState::on &&
              now - last_frame_received_ > 1500) {
            // Live sessions rely on capture delivery and native window/backend
            // health; verification pixels are never painted during operation.
            FailLocked(
                "Capture stopped delivering desktop frames; remote operation "
                "paused");
          } else if (status_.state == PrivacyState::starting &&
                     automatic_enable_ && !capture_running_) {
            if (now >= capture_start_deadline_)
              FailLocked(
                  "Desktop capture did not start; automatic privacy remains "
                  "paused");
          } else if (status_.state == PrivacyState::starting &&
                     now - started_ > 3000) {
            if (verification_phase_ == VerificationPhase::draining) {
              FailLocked(
                  "Capture did not clear its verification pixels; remote "
                  "operation paused");
            } else {
              FailLocked("Current capture backend " + capture_backend_ +
                         " did not pass privacy verification; remote operation "
                         "paused. Turn privacy off to recover.");
            }
          }
        }
      }
      if (backend_) {
        // Keep window notifications and emergency exits responsive. Low-level
        // input hooks are pumped independently by PrivacyInputGuard.
        const unsigned timeout_ms = engaged_.load() ? 25 : 2000;
        lock.unlock();
        backend_->WaitForEvents(timeout_ms);
        lock.lock();
      } else {
        // Unsupported platforms have no native health or capability polling.
        wake_.wait(lock);
      }
    }
  } catch (const std::exception& e) {
    if (!lock.owns_lock()) lock.lock();
    remote_paused_.store(true);
    SetState(PrivacyState::failed,
             std::string("Privacy component failed: ") + e.what());
  }
  if (backend_) {
    backend_->Recover();
    backend_.reset();
  }
}

}  // namespace crossdesk
