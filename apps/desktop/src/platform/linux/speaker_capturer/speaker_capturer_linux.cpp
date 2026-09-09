#include "speaker_capturer_linux.h"

#include <pulse/error.h>
#include <pulse/introspect.h>

#include <string>
#include <utility>

#include "rd_log.h"

namespace crossdesk {
namespace {
constexpr size_t kFrameSizeBytes = 480 * sizeof(int16_t);
constexpr auto kStartupTimeout = std::chrono::seconds(2);
constexpr auto kPollInterval = std::chrono::milliseconds(5);

void ReleaseOperation(pa_operation* operation) {
  if (!operation) return;
  if (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
    pa_operation_cancel(operation);
  pa_operation_unref(operation);
}
}  // namespace

SpeakerCapturerLinux::~SpeakerCapturerLinux() { Destroy(); }

int SpeakerCapturerLinux::Init(speaker_data_cb cb) {
  if (inited_) return 0;
  cb_ = std::move(cb);
  inited_ = true;
  return 0;
}

// Use nonblocking iterations instead of pa_threaded_mainloop_wait(): a server
// that never replies, disappears, or never reaches STREAM_READY must not leave
// startup/Stop waiting for a signal that will never be delivered.
bool SpeakerCapturerLinux::PumpUntil(
    const std::function<bool()>& ready,
    std::chrono::steady_clock::time_point deadline) {
  while (!stop_flag_) {
    if (ready()) return true;
    if (std::chrono::steady_clock::now() >= deadline ||
        !PA_CONTEXT_IS_GOOD(pa_context_get_state(context_)))
      return false;
    if (pa_mainloop_iterate(mainloop_, 0, nullptr) < 0) return false;
    std::this_thread::sleep_for(kPollInterval);
  }
  return false;
}

bool SpeakerCapturerLinux::Prepare() {
  const auto deadline = std::chrono::steady_clock::now() + kStartupTimeout;
  mainloop_ = pa_mainloop_new();
  if (!mainloop_) return false;
  context_ =
      pa_context_new(pa_mainloop_get_api(mainloop_), "CrossDesk system audio");
  if (!context_ ||
      pa_context_connect(context_, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
    return false;
  if (!PumpUntil(
          [&] { return pa_context_get_state(context_) == PA_CONTEXT_READY; },
          deadline))
    return false;

  struct ServerResult {
    bool done = false;
    std::string sink;
  } server;
  pa_operation* operation = pa_context_get_server_info(
      context_,
      [](pa_context*, const pa_server_info* info, void* data) {
        auto& result = *static_cast<ServerResult*>(data);
        if (info && info->default_sink_name)
          result.sink = info->default_sink_name;
        result.done = true;
      },
      &server);
  const bool got_server =
      operation && PumpUntil([&] { return server.done; }, deadline);
  ReleaseOperation(operation);
  if (!got_server || server.sink.empty()) return false;

  // Query the actual monitor name; a changed/default device need not follow
  // the "<sink>.monitor" naming convention.
  struct SinkResult {
    bool done = false;
    std::string monitor;
  } sink;
  operation = pa_context_get_sink_info_by_name(
      context_, server.sink.c_str(),
      [](pa_context*, const pa_sink_info* info, int eol, void* data) {
        auto& result = *static_cast<SinkResult*>(data);
        if (info && info->monitor_source_name)
          result.monitor = info->monitor_source_name;
        if (eol != 0) result.done = true;
      },
      &sink);
  const bool got_sink =
      operation && PumpUntil([&] { return sink.done; }, deadline);
  ReleaseOperation(operation);
  if (!got_sink || sink.monitor.empty()) return false;

  const pa_sample_spec spec{PA_SAMPLE_S16LE, 48000, 1};
  stream_ = pa_stream_new(context_, "System audio", &spec, nullptr);
  if (!stream_) return false;
  pa_stream_set_read_callback(
      stream_,
      [](pa_stream* stream, size_t, void* data) {
        static_cast<SpeakerCapturerLinux*>(data)->ReadAudio(stream);
      },
      this);
  pa_buffer_attr attr{};
  attr.maxlength = static_cast<uint32_t>(-1);
  attr.fragsize = static_cast<uint32_t>(kFrameSizeBytes);
  if (pa_stream_connect_record(stream_, sink.monitor.c_str(), &attr,
                               PA_STREAM_ADJUST_LATENCY) < 0)
    return false;
  if (!PumpUntil(
          [&] {
            const auto state = pa_stream_get_state(stream_);
            return state == PA_STREAM_READY || !PA_STREAM_IS_GOOD(state);
          },
          deadline) ||
      pa_stream_get_state(stream_) != PA_STREAM_READY)
    return false;
  LOG_INFO("System audio monitor ready: {}", sink.monitor);
  return true;
}

void SpeakerCapturerLinux::Run() {
  const bool ready = Prepare();
  running_ = ready && !stop_flag_;
  {
    std::lock_guard<std::mutex> lock(state_mtx_);
    startup_success_ = running_;
    startup_done_ = true;
  }
  startup_wake_.notify_one();
  if (!ready && !stop_flag_) {
    LOG_ERROR(
        "PulseAudio capture startup failed or timed out: {}",
        context_ ? pa_strerror(pa_context_errno(context_)) : "no context");
  }
  while (running_ && !stop_flag_) {
    if (pa_mainloop_iterate(mainloop_, 0, nullptr) < 0 ||
        pa_context_get_state(context_) != PA_CONTEXT_READY ||
        pa_stream_get_state(stream_) != PA_STREAM_READY)
      break;
    std::this_thread::sleep_for(kPollInterval);
  }
  running_ = false;
  Cleanup();
}

int SpeakerCapturerLinux::Start() {
  if (!inited_) return -1;
  if (IsRunning()) return 0;
  Stop();
  stop_flag_ = false;
  paused_ = false;
  {
    std::lock_guard<std::mutex> lock(state_mtx_);
    startup_done_ = false;
    startup_success_ = false;
  }
  mainloop_thread_ = std::thread([this] { Run(); });
  std::unique_lock<std::mutex> lock(state_mtx_);
  const bool started =
      startup_wake_.wait_for(lock, kStartupTimeout + std::chrono::seconds(1),
                             [&] { return startup_done_; }) &&
      startup_success_;
  lock.unlock();
  if (!started) {
    Stop();
    return -1;
  }
  return 0;
}

int SpeakerCapturerLinux::Stop() {
  stop_flag_ = true;
  if (mainloop_thread_.joinable()) mainloop_thread_.join();
  running_ = false;
  return 0;
}

int SpeakerCapturerLinux::Destroy() {
  Stop();
  cb_ = nullptr;
  inited_ = false;
  return 0;
}

void SpeakerCapturerLinux::ReadAudio(pa_stream* stream) {
  const void* data = nullptr;
  size_t size = 0;
  if (pa_stream_peek(stream, &data, &size) < 0 || size == 0) return;
  if (data && !paused_ && !stop_flag_ && cb_) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    frame_cache_.insert(frame_cache_.end(), bytes, bytes + size);
    size_t consumed = 0;
    while (!stop_flag_ && frame_cache_.size() - consumed >= kFrameSizeBytes) {
      cb_(frame_cache_.data() + consumed, kFrameSizeBytes, "audio");
      consumed += kFrameSizeBytes;
    }
    frame_cache_.erase(frame_cache_.begin(), frame_cache_.begin() + consumed);
  } else {
    frame_cache_.clear();
  }
  // Drop holes and paused data too, otherwise the read queue cannot advance.
  pa_stream_drop(stream);
}

void SpeakerCapturerLinux::Cleanup() {
  if (stream_) {
    pa_stream_set_read_callback(stream_, nullptr, nullptr);
    pa_stream_disconnect(stream_);
    pa_stream_unref(stream_);
    stream_ = nullptr;
  }
  if (context_) {
    pa_context_disconnect(context_);
    pa_context_unref(context_);
    context_ = nullptr;
  }
  if (mainloop_) {
    pa_mainloop_free(mainloop_);
    mainloop_ = nullptr;
  }
  frame_cache_.clear();
}

bool SpeakerCapturerLinux::IsRunning() const { return running_; }
int SpeakerCapturerLinux::Pause() {
  paused_ = true;
  return 0;
}
int SpeakerCapturerLinux::Resume() {
  paused_ = false;
  return 0;
}
}  // namespace crossdesk
