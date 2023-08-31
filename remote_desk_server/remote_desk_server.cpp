#include "remote_desk_server.h"

#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};

#define NV12_BUFFER_SIZE 1280 * 720 * 3 / 2

RemoteDeskServer ::RemoteDeskServer() {}

RemoteDeskServer ::~RemoteDeskServer() {
  if (nv12_buffer_) {
    delete nv12_buffer_;
    nv12_buffer_ = nullptr;
  }
}

int BGRAToNV12FFmpeg(unsigned char *src_buffer, int width, int height,
                     unsigned char *dst_buffer) {
  AVFrame *Input_pFrame = av_frame_alloc();
  AVFrame *Output_pFrame = av_frame_alloc();
  struct SwsContext *img_convert_ctx =
      sws_getContext(width, height, AV_PIX_FMT_BGRA, 1280, 720, AV_PIX_FMT_NV12,
                     SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  av_image_fill_arrays(Input_pFrame->data, Input_pFrame->linesize, src_buffer,
                       AV_PIX_FMT_BGRA, width, height, 1);
  av_image_fill_arrays(Output_pFrame->data, Output_pFrame->linesize, dst_buffer,
                       AV_PIX_FMT_NV12, 1280, 720, 1);

  sws_scale(img_convert_ctx, (uint8_t const **)Input_pFrame->data,
            Input_pFrame->linesize, 0, height, Output_pFrame->data,
            Output_pFrame->linesize);

  if (Input_pFrame) av_free(Input_pFrame);
  if (Output_pFrame) av_free(Output_pFrame);
  if (img_convert_ctx) sws_freeContext(img_convert_ctx);

  return 0;
}

void RemoteDeskServer::HostReceiveBuffer(const char *data, size_t size,
                                         const char *user_id,
                                         size_t user_id_size) {
  std::string msg(data, size);
  std::string user(user_id, user_id_size);

  std::cout << "Receive: [" << user << "] " << msg << std::endl;
}

int RemoteDeskServer::Init() {
  Params params;
  params.cfg_path = "../../../../config/config.ini";
  params.on_receive_buffer = [](const char *data, size_t size,
                                const char *user_id, size_t user_id_size) {
    // std::string msg(data, size);
    // std::string user(user_id, user_id_size);

    // std::cout << "Receive: [" << user << "] " << msg << std::endl;
  };

  std::string transmission_id = "000000";
  std::string user_id = "Server";
  peer = CreatePeer(&params);
  CreateConnection(peer, transmission_id.c_str(), user_id.c_str());

  nv12_buffer_ = new char[NV12_BUFFER_SIZE];

  screen_capture = new ScreenCaptureWgc();

  RECORD_DESKTOP_RECT rect;
  rect.left = 0;
  rect.top = 0;
  rect.right = GetSystemMetrics(SM_CXSCREEN);
  rect.bottom = GetSystemMetrics(SM_CYSCREEN);

  screen_capture->Init(
      rect, 60,
      [this](unsigned char *data, int size, int width, int height) -> void {
        // std::cout << "Send" << std::endl;
        BGRAToNV12FFmpeg(data, width, height, (unsigned char *)nv12_buffer_);
        SendData(peer, DATA_TYPE::VIDEO, (const char *)nv12_buffer_,
                 NV12_BUFFER_SIZE);
        // std::this_thread::sleep_for(std::chrono::milliseconds(30));
      });

  screen_capture->Start();
  return 0;
}