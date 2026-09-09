#include "speaker_capture_controller.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <utility>

#include "rd_log.h"

namespace crossdesk {

struct SpeakerCaptureController::State {
  Factory factory;
  RetryPolicy policy;
  std::mutex mutex;
  std::condition_variable wake;
  bool enabled = false;
  bool shutdown = false;
  uint64_t generation = 0;
  std::atomic<bool> running{false};

  // Never hold this mutex across a native capture operation. It only drains
  // an in-flight audio delivery when a session is disabled/destroyed.
  std::mutex callback_mutex;
  SpeakerCapturer::speaker_data_cb callback;
  bool deliver = false;
  uint64_t callback_attempt = 0;
};

SpeakerCaptureController::SpeakerCaptureController(
    Factory factory, SpeakerCapturer::speaker_data_cb cb)
    : SpeakerCaptureController(std::move(factory), std::move(cb),
                               RetryPolicy{}) {}

SpeakerCaptureController::SpeakerCaptureController(
    Factory factory, SpeakerCapturer::speaker_data_cb cb, RetryPolicy policy)
    : state_(std::make_shared<State>()) {
  state_->factory = std::move(factory);
  state_->callback = std::move(cb);
  policy.max_attempts = std::max(1u, policy.max_attempts);
  state_->policy = policy;
  // The worker owns only shared state, never this/controller/GUI. Even a
  // driver stuck inside a native API cannot block shutdown or outlive an
  // unguarded callback target. There is at most one worker per controller.
  std::thread([state = state_] { Run(state); }).detach();
}

SpeakerCaptureController::~SpeakerCaptureController() { Shutdown(); }

void SpeakerCaptureController::SetEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(state_->mutex);
  if (state_->shutdown || state_->enabled == enabled) return;
  state_->enabled = enabled;
  ++state_->generation;
  if (!enabled) {
    std::lock_guard<std::mutex> callback_lock(state_->callback_mutex);
    state_->deliver = false;
    state_->running = false;
  }
  state_->wake.notify_one();
}

bool SpeakerCaptureController::IsRunning() const { return state_->running; }

void SpeakerCaptureController::Shutdown() {
  std::lock_guard<std::mutex> lock(state_->mutex);
  state_->shutdown = true;
  state_->enabled = false;
  state_->running = false;
  {
    std::lock_guard<std::mutex> callback_lock(state_->callback_mutex);
    state_->deliver = false;
    state_->callback = nullptr;
  }
  state_->wake.notify_one();
}

void SpeakerCaptureController::Run(const std::shared_ptr<State>& state) {
  uint64_t callback_attempt = 0;
  std::unique_lock<std::mutex> lock(state->mutex);
  while (!state->shutdown) {
    state->wake.wait(lock, [&] { return state->shutdown || state->enabled; });
    if (state->shutdown) break;
    const uint64_t generation = state->generation;
    const auto cancelled = [&] {
      return state->shutdown || !state->enabled ||
             state->generation != generation;
    };

    for (unsigned attempt = 1; !cancelled(); ++attempt) {
      const uint64_t current_attempt = ++callback_attempt;
      lock.unlock();
      auto capturer = state->factory();
      std::weak_ptr<State> weak = state;
      auto callback = [weak, current_attempt](unsigned char* data, size_t size,
                                              const char* name) {
        if (auto current = weak.lock()) {
          std::lock_guard<std::mutex> callback_lock(current->callback_mutex);
          if (current->deliver &&
              current->callback_attempt == current_attempt &&
              current->callback) {
            current->callback(data, size, name);
          }
        }
      };
      const bool initialized = capturer && capturer->Init(callback) == 0;
      lock.lock();
      const bool should_start = initialized && !cancelled();
      lock.unlock();
      const bool started = should_start && capturer->Start() == 0;
      lock.lock();
      if (started && !cancelled()) {
        {
          std::lock_guard<std::mutex> callback_lock(state->callback_mutex);
          state->callback_attempt = current_attempt;
          state->deliver = true;
          state->running = true;
        }
        LOG_INFO("System audio capture started (attempt {})", attempt);
        while (!cancelled()) {
          if (state->wake.wait_for(lock, state->policy.health_interval,
                                   cancelled))
            break;
          lock.unlock();
          const bool healthy = capturer->IsRunning();
          lock.lock();
          if (!healthy) {
            LOG_WARN("System audio capture stopped unexpectedly");
            break;
          }
        }
      }
      {
        std::lock_guard<std::mutex> callback_lock(state->callback_mutex);
        state->deliver = false;
        state->running = false;
      }
      lock.unlock();
      // Recreate on every attempt/reconnection to discard stale displays,
      // PulseAudio sources and invalidated WASAPI devices.
      if (capturer) capturer->Destroy();
      capturer.reset();
      lock.lock();
      if (cancelled()) break;
      if (attempt >= state->policy.max_attempts) {
        LOG_ERROR(
            "System audio capture disabled after {} attempts; toggle audio "
            "or reconnect to retry",
            attempt);
        state->wake.wait(lock, cancelled);
        break;
      }
      const auto delay =
          state->policy.initial_delay * (1u << std::min(attempt - 1, 5u));
      LOG_WARN("System audio capture failed (attempt {}); retry in {} ms",
               attempt, delay.count());
      state->wake.wait_for(lock, delay, cancelled);
    }
  }
}

}  // namespace crossdesk
