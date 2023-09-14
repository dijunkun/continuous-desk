#ifndef _REMOTE_DESK_SERVER_H_
#define _REMOTE_DESK_SERVER_H_

#include "screen_capture_wgc.h"
#include "x.h"

class RemoteDeskServer {
 public:
  RemoteDeskServer();
  ~RemoteDeskServer();

 public:
  int Init();

  static void ReceiveVideoBuffer(const char* data, size_t size,
                                 const char* user_id, size_t user_id_size);
  static void ReceiveAudioBuffer(const char* data, size_t size,
                                 const char* user_id, size_t user_id_size);
  static void ReceiveDataBuffer(const char* data, size_t size,
                                const char* user_id, size_t user_id_size);

 private:
  PeerPtr* peer = nullptr;
  ScreenCaptureWgc* screen_capture = nullptr;

  char* nv12_buffer_ = nullptr;
  std::chrono::steady_clock::time_point last_frame_time_;
};

#endif