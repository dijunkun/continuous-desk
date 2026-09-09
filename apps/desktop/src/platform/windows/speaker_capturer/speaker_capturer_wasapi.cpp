#include "speaker_capturer_wasapi.h"

#include <utility>

#include "rd_log.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace crossdesk {

// Each attempt owns its device. A failed/old session must never uninitialize
// a device belonging to another capturer or reuse already released storage.
class SpeakerCapturerWasapi::Impl {
 public:
  ma_device device{};
  bool initialized = false;
  speaker_data_cb callback;
};

SpeakerCapturerWasapi::SpeakerCapturerWasapi() : impl_(new Impl) {}
SpeakerCapturerWasapi::~SpeakerCapturerWasapi() {
  Destroy();
  delete impl_;
}

int SpeakerCapturerWasapi::Init(speaker_data_cb cb) {
  if (impl_->initialized) return 0;
  impl_->callback = std::move(cb);
  ma_device_config config = ma_device_config_init(ma_device_type_loopback);
  config.capture.pDeviceID =
      nullptr;  // Resolve the current default each attempt.
  config.capture.format = ma_format_s16;
  config.capture.channels = 1;
  config.sampleRate = 48000;
  config.pUserData = impl_;
  config.dataCallback = [](ma_device* device, void*, const void* input,
                           ma_uint32 frames) {
    auto* state = static_cast<Impl*>(device->pUserData);
    if (state && state->callback && input && frames > 0)
      state->callback((unsigned char*)input, frames * sizeof(int16_t), "audio");
  };
  ma_backend backends[] = {ma_backend_wasapi};
  const ma_result result =
      ma_device_init_ex(backends, 1, nullptr, &config, &impl_->device);
  if (result != MA_SUCCESS) {
    impl_->callback = nullptr;
    LOG_ERROR("Failed to initialize WASAPI loopback device: {}",
              static_cast<int>(result));
    return -1;
  }
  impl_->initialized = true;
  return 0;
}

int SpeakerCapturerWasapi::Start() {
  if (!impl_->initialized) return -1;
  if (IsRunning()) return 0;
  const ma_result result = ma_device_start(&impl_->device);
  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed to start WASAPI loopback device: {}",
              static_cast<int>(result));
    Destroy();
    return -1;
  }
  return 0;
}

int SpeakerCapturerWasapi::Stop() {
  if (!impl_->initialized || !IsRunning()) return 0;
  const ma_result result = ma_device_stop(&impl_->device);
  if (result != MA_SUCCESS) {
    LOG_WARN("Failed to stop WASAPI loopback device: {}",
             static_cast<int>(result));
    return -1;
  }
  return 0;
}

int SpeakerCapturerWasapi::Destroy() {
  if (impl_->initialized) {
    // ma_device_uninit also stops and drains the device callback thread.
    ma_device_uninit(&impl_->device);
    impl_->initialized = false;
  }
  impl_->callback = nullptr;
  return 0;
}

bool SpeakerCapturerWasapi::IsRunning() const {
  return impl_->initialized && ma_device_is_started(&impl_->device);
}
int SpeakerCapturerWasapi::Pause() { return Stop(); }
int SpeakerCapturerWasapi::Resume() { return Start(); }
}  // namespace crossdesk
