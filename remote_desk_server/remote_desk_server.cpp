#include "remote_desk_server.h"

#ifdef _WIN32
#include <Winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#endif

#define SDL_MAIN_HANDLED

#include <fstream>
#include <iostream>

#include "SDL2/SDL.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};

#define NV12_BUFFER_SIZE 1280 * 720 * 3 / 2

int screen_w = 0;
int screen_h = 0;

typedef enum { mouse = 0, keyboard } ControlType;
typedef enum { move = 0, left_down, left_up, right_down, right_up } MouseFlag;
typedef enum { key_down = 0, key_up } KeyFlag;
typedef struct {
  size_t x;
  size_t y;
  MouseFlag flag;
} Mouse;

typedef struct {
  size_t key_value;
  KeyFlag flag;
} Key;

typedef struct {
  ControlType type;
  union {
    Mouse m;
    Key k;
  };
} RemoteAction;

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

void RemoteDeskServer::ReceiveVideoBuffer(const char *data, size_t size,
                                          const char *user_id,
                                          size_t user_id_size) {
  std::string msg(data, size);
  std::string user(user_id, user_id_size);

  std::cout << "Receive: [" << user << "] " << msg << std::endl;
}

void RemoteDeskServer::ReceiveAudioBuffer(const char *data, size_t size,
                                          const char *user_id,
                                          size_t user_id_size) {}

void RemoteDeskServer::ReceiveDataBuffer(const char *data, size_t size,
                                         const char *user_id,
                                         size_t user_id_size) {
  std::string user(user_id, user_id_size);

  RemoteAction remote_action;
  memcpy(&remote_action, data, sizeof(remote_action));

  INPUT ip;

  if (remote_action.type == ControlType::mouse) {
    ip.type = INPUT_MOUSE;
    ip.mi.dx = remote_action.m.x * screen_w / 1280;
    ip.mi.dy = remote_action.m.y * screen_h / 720;
    if (remote_action.m.flag == MouseFlag::left_down) {
      ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE;
    } else if (remote_action.m.flag == MouseFlag::left_up) {
      ip.mi.dwFlags = MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE;
    } else if (remote_action.m.flag == MouseFlag::right_down) {
      ip.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_ABSOLUTE;
    } else if (remote_action.m.flag == MouseFlag::right_up) {
      ip.mi.dwFlags = MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_ABSOLUTE;
    }
    ip.mi.mouseData = 0;
    ip.mi.time = 0;

    // Send the press
    SendInput(1, &ip, sizeof(INPUT));

    std::cout << "Receive data from [" << user << "],  " << ip.type << " "
              << ip.mi.dwFlags << " " << ip.mi.dx << " " << ip.mi.dy
              << std::endl;
  }
}

std::string GetMac(char *mac_addr) {
  int len = 0;
#ifdef _WIN32
  IP_ADAPTER_INFO adapterInfo[16];
  DWORD bufferSize = sizeof(adapterInfo);

  DWORD result = GetAdaptersInfo(adapterInfo, &bufferSize);
  if (result == ERROR_SUCCESS) {
    PIP_ADAPTER_INFO adapter = adapterInfo;
    while (adapter) {
      for (UINT i = 0; i < adapter->AddressLength; i++) {
        len += sprintf(mac_addr + len, "%.2X", adapter->Address[i]);
      }
      break;
    }
  }
#else
  std::string ifName = "en0";

  struct ifaddrs *addrs;
  struct ifaddrs *cursor;
  const struct sockaddr_dl *dlAddr;

  if (!getifaddrs(&addrs)) {
    cursor = addrs;
    while (cursor != 0) {
      const struct sockaddr_dl *socAddr =
          (const struct sockaddr_dl *)cursor->ifa_addr;
      if ((cursor->ifa_addr->sa_family == AF_LINK) &&
          (socAddr->sdl_type == IFT_ETHER) &&
          strcmp("en0", cursor->ifa_name) == 0) {
        dlAddr = (const struct sockaddr_dl *)cursor->ifa_addr;
        const unsigned char *base =
            (const unsigned char *)&dlAddr->sdl_data[dlAddr->sdl_nlen];
        for (int i = 0; i < dlAddr->sdl_alen; i++) {
          len += sprintf(mac_addr + len, "%.2X", base[i]);
        }
      }
      cursor = cursor->ifa_next;
    }
    freeifaddrs(addrs);
  }
#endif
  return mac_addr;
}

int RemoteDeskServer::Init() {
  std::string default_cfg_path = "../../../../config/config.ini";
  std::ifstream f(default_cfg_path.c_str());

  Params params;
  params.cfg_path = f.good() ? "../../../../config/config.ini" : "config.ini";
  params.on_receive_video_buffer = ReceiveVideoBuffer;
  params.on_receive_audio_buffer = ReceiveAudioBuffer;
  params.on_receive_data_buffer = ReceiveDataBuffer;

  std::string transmission_id = "000001";
  char mac_addr[10];
  std::string user_id = "S-" + std::string(GetMac(mac_addr));
  peer = CreatePeer(&params);
  CreateConnection(peer, transmission_id.c_str(), user_id.c_str());

  nv12_buffer_ = new char[NV12_BUFFER_SIZE];

  screen_capture = new ScreenCaptureWgc();

  RECORD_DESKTOP_RECT rect;
  rect.left = 0;
  rect.top = 0;
  rect.right = GetSystemMetrics(SM_CXSCREEN);
  rect.bottom = GetSystemMetrics(SM_CYSCREEN);

  screen_w = GetSystemMetrics(SM_CXSCREEN);
  screen_h = GetSystemMetrics(SM_CYSCREEN);

  last_frame_time_ = std::chrono::high_resolution_clock::now();
  screen_capture->Init(
      rect, 60,
      [this](unsigned char *data, int size, int width, int height) -> void {
        // std::cout << "Send" << std::endl;

        auto now_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = now_time - last_frame_time_;
        auto tc = duration.count() * 1000;

        if (tc >= 0) {
          BGRAToNV12FFmpeg(data, width, height, (unsigned char *)nv12_buffer_);
          SendData(peer, DATA_TYPE::VIDEO, (const char *)nv12_buffer_,
                   NV12_BUFFER_SIZE);
          last_frame_time_ = now_time;
        }
      });

  screen_capture->Start();
  return 0;
}