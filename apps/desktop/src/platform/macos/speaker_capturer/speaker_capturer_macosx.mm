#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

#include "rd_log.h"
#include "speaker_capturer_macosx.h"

namespace {
constexpr auto kCaptureTimeout = std::chrono::seconds(2);

std::string NSErrorToString(NSError* error) {
  if (!error) return "";
  NSString* message = [NSString
      stringWithFormat:@"%@ (%@:%ld), reason=%@", error.localizedDescription, error.domain,
                       (long)error.code, error.localizedFailureReason ?: @""];
  return message.UTF8String ?: "";
}

// Late system replies own only their result, never the capturer or stack data.
// Complete() also tells a late successful start to stop its abandoned stream.
template <typename T>
class CaptureReply {
 public:
  bool Complete(T value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (expired_) return false;
    value_ = std::move(value);
    ready_ = true;
    wake_.notify_one();
    return true;
  }
  bool Wait(T& value) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!wake_.wait_for(lock, kCaptureTimeout, [&] { return ready_; })) {
      expired_ = true;
      return false;
    }
    value = value_;
    return true;
  }

 private:
  std::mutex mutex_;
  std::condition_variable wake_;
  bool ready_ = false;
  bool expired_ = false;
  T value_{};
};

struct ContentReply {
  SCShareableContent* content = nil;
  NSError* error = nil;
};
struct StreamReply {
  NSError* error = nil;
};
struct AudioCallbackState {
  std::mutex mutex;
  crossdesk::SpeakerCapturer::speaker_data_cb callback;
  std::atomic<bool> running{false};
};
}  // namespace

@interface SpeakerCaptureDelegate : NSObject <SCStreamDelegate, SCStreamOutput> {
  std::shared_ptr<AudioCallbackState> state_;
}
- (instancetype)initWithState:(std::shared_ptr<AudioCallbackState>)state;
@end

@implementation SpeakerCaptureDelegate
- (instancetype)initWithState:(std::shared_ptr<AudioCallbackState>)state {
  self = [super init];
  if (self) state_ = std::move(state);
  return self;
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error {
  state_->running = false;
  SPDLOG_LOGGER_WARN(crossdesk::get_logger(), "System audio stream stopped: {}",
                     NSErrorToString(error));
}

- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                   ofType:(SCStreamOutputType)type {
  if (type != SCStreamOutputTypeAudio) return;
  const auto state = state_;
  std::lock_guard<std::mutex> lock(state->mutex);
  if (!state->callback || !state->running) return;

  CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
  if (!blockBuffer) {
    return;
  }

  size_t length = CMBlockBufferGetDataLength(blockBuffer);
  char* dataPtr = NULL;
  OSStatus dataStatus = CMBlockBufferGetDataPointer(blockBuffer, 0, NULL, NULL, &dataPtr);
  if (dataStatus != noErr || dataPtr == nullptr || length == 0) {
    return;
  }

  CMAudioFormatDescriptionRef formatDesc = CMSampleBufferGetFormatDescription(sampleBuffer);
  if (!formatDesc) {
    return;
  }

  const AudioStreamBasicDescription* asbd =
      CMAudioFormatDescriptionGetStreamBasicDescription(formatDesc);
  if (!asbd || asbd->mChannelsPerFrame == 0) {
    return;
  }

  if (state->callback) {
    std::vector<short> out_pcm16;
    if (asbd->mFormatFlags & kAudioFormatFlagIsFloat) {
      int channels = asbd->mChannelsPerFrame;
      int samples = (int)(length / sizeof(float));
      float* floatData = (float*)dataPtr;
      std::vector<short> pcm16(samples);
      for (int i = 0; i < samples; ++i) {
        float v = floatData[i];
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        pcm16[i] = (short)(v * 32767.0f);
      }

      if (channels > 1) {
        int mono_samples = samples / channels;
        out_pcm16.resize(mono_samples);
        for (int i = 0; i < mono_samples; ++i) {
          int sum = 0;
          for (int c = 0; c < channels; ++c) {
            sum += pcm16[i * channels + c];
          }
          out_pcm16[i] = sum / channels;
        }
      } else {
        out_pcm16 = std::move(pcm16);
      }
    } else if (asbd->mBitsPerChannel == 16) {
      int channels = asbd->mChannelsPerFrame;
      int samples = (int)(length / 2);
      short* src = (short*)dataPtr;
      if (channels > 1) {
        int mono_samples = samples / channels;
        out_pcm16.resize(mono_samples);
        for (int i = 0; i < mono_samples; ++i) {
          int sum = 0;
          for (int c = 0; c < channels; ++c) {
            sum += src[i * channels + c];
          }
          out_pcm16[i] = sum / channels;
        }
      } else {
        out_pcm16.assign(src, src + samples);
      }
    }

    size_t frame_bytes = 960;  // 480 * 2
    size_t total_bytes = out_pcm16.size() * sizeof(short);
    unsigned char* p = (unsigned char*)out_pcm16.data();
    for (size_t offset = 0; offset + frame_bytes <= total_bytes; offset += frame_bytes) {
      if (!state->callback) {
        return;
      }
      state->callback(p + offset, frame_bytes, "audio");
    }
  }
}
@end

namespace crossdesk {
class SpeakerCapturerMacosx::Impl {
 public:
  speaker_data_cb callback;
  SCStream* stream = nil;
  SpeakerCaptureDelegate* delegate = nil;
  dispatch_queue_t queue = nil;
  std::shared_ptr<AudioCallbackState> state;
};

SpeakerCapturerMacosx::SpeakerCapturerMacosx() : impl_(new Impl) {}
SpeakerCapturerMacosx::~SpeakerCapturerMacosx() {
  Destroy();
  delete impl_;
}

int SpeakerCapturerMacosx::Init(speaker_data_cb cb) {
  impl_->callback = std::move(cb);
  if (!impl_->queue)
    impl_->queue = dispatch_queue_create("SpeakerAudio.Queue", DISPATCH_QUEUE_SERIAL);
  return 0;
}

int SpeakerCapturerMacosx::Start() {
  @autoreleasepool {
    if (!impl_->callback) return -1;
    if (IsRunning()) return 0;
    if (Stop() != 0) return -1;

    // A cached SCDisplay becomes invalid after hotplug, sleep or a display
    // reconfiguration. Obtain a fresh snapshot on every start/reconnection.
    auto content_reply = std::make_shared<CaptureReply<ContentReply>>();
    [SCShareableContent
        getShareableContentWithCompletionHandler:^(SCShareableContent* content, NSError* error) {
          content_reply->Complete({content, error});
        }];
    ContentReply content;
    if (!content_reply->Wait(content)) {
      LOG_ERROR("Timed out refreshing system audio capture displays");
      return -1;
    }
    if (content.error || !content.content) {
      LOG_ERROR("Failed to refresh system audio capture displays: {}",
                NSErrorToString(content.error));
      return -1;
    }
    SCDisplay* display = nil;
    const auto main_id = CGMainDisplayID();
    for (SCDisplay* candidate in content.content.displays) {
      if (candidate.displayID == main_id) {
        display = candidate;
        break;
      }
    }
    if (!display) display = content.content.displays.firstObject;
    if (!display) {
      LOG_ERROR("No display available for system audio capture");
      return -1;
    }
    LOG_INFO("Starting system audio capture with current display {}", display.displayID);

    SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
    config.capturesAudio = YES;
    config.sampleRate = 48000;
    config.channelCount = 1;
    SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:display
                                                      excludingWindows:@[]];
    impl_->state = std::make_shared<AudioCallbackState>();
    impl_->state->callback = impl_->callback;
    impl_->delegate = [[SpeakerCaptureDelegate alloc] initWithState:impl_->state];
    impl_->stream = [[SCStream alloc] initWithFilter:filter
                                       configuration:config
                                            delegate:impl_->delegate];
    NSError* output_error = nil;
    if (!impl_->stream || ![impl_->stream addStreamOutput:impl_->delegate
                                                     type:SCStreamOutputTypeAudio
                                       sampleHandlerQueue:impl_->queue
                                                    error:&output_error]) {
      LOG_ERROR("Failed to attach system audio output: {}", NSErrorToString(output_error));
      Stop();
      return -1;
    }

    auto start_reply = std::make_shared<CaptureReply<StreamReply>>();
    SCStream* stream = impl_->stream;
    impl_->state->running = true;
    [stream startCaptureWithCompletionHandler:^(NSError* error) {
      if (!start_reply->Complete({error}) && !error) {
        [stream stopCaptureWithCompletionHandler:nil];
      }
    }];
    StreamReply started;
    if (!start_reply->Wait(started)) {
      LOG_ERROR("Timed out starting system audio capture");
      Stop();
      return -1;
    }
    if (started.error || !impl_->state->running) {
      LOG_ERROR("Failed to start system audio capture: {}", NSErrorToString(started.error));
      Stop();
      return -1;
    }
    return 0;
  }
}

int SpeakerCapturerMacosx::Stop() {
  @autoreleasepool {
    if (auto state = std::exchange(impl_->state, {})) {
      std::lock_guard<std::mutex> lock(state->mutex);
      state->running = false;
      state->callback = nullptr;
    }
    SCStream* stream = impl_->stream;
    impl_->stream = nil;
    impl_->delegate = nil;
    if (!stream) return 0;
    auto stop_reply = std::make_shared<CaptureReply<StreamReply>>();
    [stream stopCaptureWithCompletionHandler:^(NSError* error) {
      stop_reply->Complete({error});
    }];
    StreamReply stopped;
    if (!stop_reply->Wait(stopped)) {
      LOG_ERROR("Timed out stopping system audio capture; callbacks revoked");
      return -1;
    }
    // A start failure or daemon restart can leave an already stopped stream.
    // Its references have been discarded, so future starts can recover.
    if (stopped.error) LOG_WARN("System audio stop reply: {}", NSErrorToString(stopped.error));
    return 0;
  }
}

bool SpeakerCapturerMacosx::IsRunning() const { return impl_->state && impl_->state->running; }

int SpeakerCapturerMacosx::Destroy() {
  const int result = Stop();
  impl_->callback = nullptr;
  impl_->queue = nil;
  return result;
}
int SpeakerCapturerMacosx::Pause() { return Stop(); }
int SpeakerCapturerMacosx::Resume() { return Start(); }
}  // namespace crossdesk
