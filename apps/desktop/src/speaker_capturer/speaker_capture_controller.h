/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-09
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _SPEAKER_CAPTURE_CONTROLLER_H_
#define _SPEAKER_CAPTURE_CONTROLLER_H_

#include <chrono>
#include <functional>
#include <memory>

#include "speaker_capturer.h"

namespace crossdesk {

// Serializes platform capture operations away from the UI and transport
// threads. Stop/Shutdown revoke callbacks before returning; native teardown
// can then finish without keeping the GUI or its peer alive.
class SpeakerCaptureController {
 public:
  using Factory = std::function<std::unique_ptr<SpeakerCapturer>()>;
  struct RetryPolicy {
    unsigned max_attempts = 3;
    std::chrono::milliseconds initial_delay{1000};
    std::chrono::milliseconds health_interval{100};
  };

  SpeakerCaptureController(Factory factory,
                           SpeakerCapturer::speaker_data_cb cb);
  SpeakerCaptureController(Factory factory, SpeakerCapturer::speaker_data_cb cb,
                           RetryPolicy policy);
  ~SpeakerCaptureController();
  SpeakerCaptureController(const SpeakerCaptureController&) = delete;
  SpeakerCaptureController& operator=(const SpeakerCaptureController&) = delete;

  void SetEnabled(bool enabled);
  bool IsRunning() const;
  void Shutdown();

 private:
  struct State;
  static void Run(const std::shared_ptr<State>& state);
  std::shared_ptr<State> state_;
};

}  // namespace crossdesk

#endif