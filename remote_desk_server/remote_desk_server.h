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

  static void HostReceiveBuffer(const char* data, size_t size,
                                const char* user_id, size_t user_id_size);

 private:
  PeerPtr* peer = nullptr;
  ScreenCaptureWgc* screen_capture = nullptr;

  char* nv12_buffer_ = nullptr;
};

#endif