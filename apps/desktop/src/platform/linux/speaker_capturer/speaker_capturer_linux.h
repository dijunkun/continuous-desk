/*
 * @Author: DI JUNKUN
 * @Date: 2025-07-15
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef CROSSDESK_SPEAKER_CAPTURER_LINUX_H_
#define CROSSDESK_SPEAKER_CAPTURER_LINUX_H_

#include <pulse/pulseaudio.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "speaker_capturer.h"

namespace crossdesk {
class SpeakerCapturerLinux : public SpeakerCapturer {
 public:
  SpeakerCapturerLinux() = default;
  ~SpeakerCapturerLinux() override;
  int Init(speaker_data_cb cb) override;
  int Destroy() override;
  int Start() override;
  int Stop() override;
  bool IsRunning() const override;
  int Pause();
  int Resume();

 private:
  void Run();
  bool Prepare();
  bool PumpUntil(const std::function<bool()>& ready,
                 std::chrono::steady_clock::time_point deadline);
  void Cleanup();
  void ReadAudio(pa_stream* stream);

  speaker_data_cb cb_;
  bool inited_ = false;
  std::atomic<bool> paused_{false};
  std::atomic<bool> stop_flag_{false};
  std::atomic<bool> running_{false};
  std::thread mainloop_thread_;
  std::mutex state_mtx_;
  std::condition_variable startup_wake_;
  bool startup_done_ = false;
  bool startup_success_ = false;

  // These objects are created, driven and freed exclusively by Run(). Stop
  // only changes an atomic flag and joins, never races PulseAudio callbacks.
  pa_mainloop* mainloop_ = nullptr;
  pa_context* context_ = nullptr;
  pa_stream* stream_ = nullptr;
  std::vector<uint8_t> frame_cache_;
};
}  // namespace crossdesk
#endif
