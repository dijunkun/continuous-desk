#include <SDL.h>
#include <stdio.h>
#ifdef _WIN32
// #pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#pragma comment(linker, "/subsystem:\"console\"")
#include <Winsock2.h>
#include <iphlpapi.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <sys/socket.h>
#include <sys/types.h>
#elif __linux__
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
};

#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "log.h"
#ifdef _WIN32
#include "screen_capture_wgc.h"
#elif __linux__
#include "screen_capture_x11.h"
#elif __APPLE__
#include "screen_capture_avf.h"
#endif
#include "../../thirdparty/projectx/src/interface/x.h"

#define NV12_BUFFER_SIZE 1280 * 720 * 3 / 2

#ifdef REMOTE_DESK_DEBUG
#define MOUSE_CONTROL 0
#else
#define MOUSE_CONTROL 1
#endif

int screen_w = 1280, screen_h = 720;
int window_w = 1280, window_h = 720;
const int pixel_w = 1280, pixel_h = 720;

unsigned char dst_buffer[pixel_w * pixel_h * 3 / 2];
unsigned char audio_buffer[960];
SDL_Texture *sdlTexture = nullptr;
SDL_Renderer *sdlRenderer = nullptr;
SDL_Rect sdlRect;
SDL_Window *window;
static SDL_AudioDeviceID input_dev;
static SDL_AudioDeviceID output_dev;

uint32_t start_time, end_time, elapsed_time;
uint32_t frame_count = 0;
int fps = 0;

#if 1
static Uint8 *buffer = 0;
static int in_pos = 0;
static int out_pos = 0;
static std::atomic<bool> audio_buffer_fresh = false;
static uint32_t last_ts = 0;

char *out = "audio_old.pcm";
FILE *outfile = fopen(out, "wb+");

int64_t src_ch_layout = AV_CH_LAYOUT_MONO;
int src_rate = 48000;
enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16;
int src_nb_channels = 0;
uint8_t **src_data = NULL;  // 二级指针
int src_linesize;
int src_nb_samples = 480;

// 输出参数
int64_t dst_ch_layout = AV_CH_LAYOUT_MONO;
int dst_rate = 48000;
enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;
int dst_nb_channels = 0;
uint8_t **dst_data = NULL;  // 二级指针
int dst_linesize;
int dst_nb_samples;
int max_dst_nb_samples;

// 输出文件
const char *dst_filename = NULL;  // 保存输出的pcm到本地，然后播放验证
FILE *dst_file;

int dst_bufsize;

// 重采样实例
struct SwrContext *swr_ctx;

double t;
int ret;
#endif

int audio_len = 0;
#define MAX_FRAME_SIZE 6 * 960
#define CHANNELS 2
unsigned char pcm_bytes[MAX_FRAME_SIZE * CHANNELS * 2];

std::string window_title = "Remote Desk Client";

std::string server_connection_status_str = "-";
std::string client_connection_status_str = "-";
std::string server_signal_status_str = "-";
std::string client_signal_status_str = "-";

std::atomic<ConnectionStatus> server_connection_status =
    ConnectionStatus::Closed;
std::atomic<ConnectionStatus> client_connection_status =
    ConnectionStatus::Closed;
std::atomic<SignalStatus> server_signal_status = SignalStatus::SignalClosed;
std::atomic<SignalStatus> client_signal_status = SignalStatus::SignalClosed;

// Refresh Event
#define REFRESH_EVENT (SDL_USEREVENT + 1)
#define QUIT_EVENT (SDL_USEREVENT + 2)

typedef struct {
  char password[16];
} CDCache;

int thread_exit = 0;
PeerPtr *peer_server = nullptr;
PeerPtr *peer_client = nullptr;
bool joined = false;
bool received_frame = false;

static bool connect_button_pressed = false;
static const char *connect_label = "Connect";
static char input_password[7] = "";
static FILE *cd_cache_file = nullptr;
static CDCache cd_cache;
static char mac_addr[16];
static bool is_create_connection = false;
static bool done = false;

#ifdef _WIN32
ScreenCaptureWgc *screen_capture = nullptr;
#elif __linux__
ScreenCaptureX11 *screen_capture = nullptr;
#elif __APPLE__
ScreenCaptureAvf *screen_capture = nullptr;
#endif

char *nv12_buffer = nullptr;

#ifdef __linux__
std::chrono::_V2::system_clock::time_point last_frame_time_;
#else
std::chrono::steady_clock::time_point last_frame_time_;
#endif

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

inline int ProcessMouseKeyEven(SDL_Event &ev) {
  float ratio = 1280.0 / window_w;

  RemoteAction remote_action;

  if (SDL_KEYDOWN == ev.type)  // SDL_KEYUP
  {
    // printf("SDLK_DOWN: %d\n", SDL_KeyCode(ev.key.keysym.sym));
    if (SDLK_DOWN == ev.key.keysym.sym) {
      // printf("SDLK_DOWN  \n");

    } else if (SDLK_UP == ev.key.keysym.sym) {
      // printf("SDLK_UP  \n");

    } else if (SDLK_LEFT == ev.key.keysym.sym) {
      // printf("SDLK_LEFT  \n");

    } else if (SDLK_RIGHT == ev.key.keysym.sym) {
      // printf("SDLK_RIGHT  \n");
    }
  } else if (SDL_MOUSEBUTTONDOWN == ev.type) {
    if (SDL_BUTTON_LEFT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      // printf("SDL_MOUSEBUTTONDOWN x, y %d %d  \n", px, py);

      remote_action.type = ControlType::mouse;
      remote_action.m.flag = MouseFlag::left_down;
      remote_action.m.x = ev.button.x * ratio;
      remote_action.m.y = ev.button.y * ratio;

      SendData(peer_client, DATA_TYPE::DATA, (const char *)&remote_action,
               sizeof(remote_action));

    } else if (SDL_BUTTON_RIGHT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      // printf("SDL_BUTTON_RIGHT x, y %d %d  \n", px, py);

      remote_action.type = ControlType::mouse;
      remote_action.m.flag = MouseFlag::right_down;
      remote_action.m.x = ev.button.x * ratio;
      remote_action.m.y = ev.button.y * ratio;

      SendData(peer_client, DATA_TYPE::DATA, (const char *)&remote_action,
               sizeof(remote_action));
    }
  } else if (SDL_MOUSEBUTTONUP == ev.type) {
    if (SDL_BUTTON_LEFT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      // printf("SDL_MOUSEBUTTONUP x, y %d %d  \n", px, py);

      remote_action.type = ControlType::mouse;
      remote_action.m.flag = MouseFlag::left_up;
      remote_action.m.x = ev.button.x * ratio;
      remote_action.m.y = ev.button.y * ratio;

      SendData(peer_client, DATA_TYPE::DATA, (const char *)&remote_action,
               sizeof(remote_action));

    } else if (SDL_BUTTON_RIGHT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      // printf("SDL_MOUSEBUTTONUP x, y %d %d  \n", px, py);

      remote_action.type = ControlType::mouse;
      remote_action.m.flag = MouseFlag::right_up;
      remote_action.m.x = ev.button.x * ratio;
      remote_action.m.y = ev.button.y * ratio;

      SendData(peer_client, DATA_TYPE::DATA, (const char *)&remote_action,
               sizeof(remote_action));
    }
  } else if (SDL_MOUSEMOTION == ev.type) {
    int px = ev.motion.x;
    int py = ev.motion.y;

    // printf("SDL_MOUSEMOTION x, y %d %d  \n", px, py);

    remote_action.type = ControlType::mouse;
    remote_action.m.flag = MouseFlag::move;
    remote_action.m.x = ev.button.x * ratio;
    remote_action.m.y = ev.button.y * ratio;

    SendData(peer_client, DATA_TYPE::DATA, (const char *)&remote_action,
             sizeof(remote_action));
  } else if (SDL_QUIT == ev.type) {
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
    printf("SDL_QUIT\n");
    return 0;
  }

  return 0;
}

void SdlCaptureAudioIn(void *userdata, Uint8 *stream, int len) {
  int64_t delay = swr_get_delay(swr_ctx, src_rate);
  dst_nb_samples =
      av_rescale_rnd(delay + src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
  if (dst_nb_samples > max_dst_nb_samples) {
    av_freep(&dst_data[0]);
    ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
                           dst_nb_samples, dst_sample_fmt, 1);
    if (ret < 0) return;
    max_dst_nb_samples = dst_nb_samples;
  }

  ret = swr_convert(swr_ctx, dst_data, dst_nb_samples,
                    (const uint8_t **)&stream, src_nb_samples);
  if (ret < 0) {
    fprintf(stderr, "Error while converting\n");
    return;
  }
  dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels, ret,
                                           dst_sample_fmt, 1);
  if (dst_bufsize < 0) {
    fprintf(stderr, "Could not get sample buffer size\n");
    return;
  }

  if (1) {
    if ("ClientConnected" == client_connection_status_str) {
      SendData(peer_client, DATA_TYPE::AUDIO, (const char *)dst_data[0],
               dst_bufsize);
    }

    if ("ServerConnected" == server_connection_status_str) {
      SendData(peer_server, DATA_TYPE::AUDIO, (const char *)dst_data[0],
               dst_bufsize);
    }
  } else {
    memcpy(audio_buffer, dst_data[0], dst_bufsize);
    audio_len = dst_bufsize;
    SDL_Delay(10);
    audio_buffer_fresh = true;
  }
}

void SdlCaptureAudioOut(void *userdata, Uint8 *stream, int len) {
  // if ("ClientConnected" != client_connection_status_str) {
  //   return;
  // }

  if (!audio_buffer_fresh) {
    return;
  }

  SDL_memset(stream, 0, len);

  if (audio_len == 0) {
    return;
  } else {
  }

  len = (len > audio_len ? audio_len : len);
  SDL_MixAudioFormat(stream, audio_buffer, AUDIO_S16LSB, len,
                     SDL_MIX_MAXVOLUME);
  audio_buffer_fresh = false;
}

void ServerReceiveVideoBuffer(const char *data, size_t size,
                              const char *user_id, size_t user_id_size) {}

void ClientReceiveVideoBuffer(const char *data, size_t size,
                              const char *user_id, size_t user_id_size) {
  // std::cout << "Receive: [" << user_id << "] " << std::endl;
  if (joined) {
    memcpy(dst_buffer, data, size);

    SDL_Event event;
    event.type = REFRESH_EVENT;
    SDL_PushEvent(&event);
    received_frame = true;
  }
}

void ServerReceiveAudioBuffer(const char *data, size_t size,
                              const char *user_id, size_t user_id_size) {
  // memset(audio_buffer, 0, size);
  // memcpy(audio_buffer, data, size);
  // audio_len = size;
  audio_buffer_fresh = true;

  SDL_QueueAudio(output_dev, data, size);
  // printf("queue audio\n");
}

void ClientReceiveAudioBuffer(const char *data, size_t size,
                              const char *user_id, size_t user_id_size) {
  // std::cout << "Client receive audio, size " << size << ", user [" << user_id
  //           << "] " << std::endl;
  SDL_QueueAudio(output_dev, data, size);
}

void ServerReceiveDataBuffer(const char *data, size_t size, const char *user_id,
                             size_t user_id_size) {
  std::string user(user_id, user_id_size);

  RemoteAction remote_action;
  memcpy(&remote_action, data, sizeof(remote_action));

  // std::cout << "remote_action: " << remote_action.type << " "
  //           << remote_action.m.flag << " " << remote_action.m.x << " "
  //           << remote_action.m.y << std::endl;

  int mouse_pos_x = remote_action.m.x * screen_w / 1280;
  int mouse_pos_y = remote_action.m.y * screen_h / 720;
#if 1
#ifdef _WIN32
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
    } else {
      ip.mi.dwFlags = MOUSEEVENTF_MOVE;
    }
    ip.mi.mouseData = 0;
    ip.mi.time = 0;

    // Set cursor pos
    SetCursorPos(ip.mi.dx, ip.mi.dy);
    // Send the press
    if (ip.mi.dwFlags != MOUSEEVENTF_MOVE) {
      SendInput(1, &ip, sizeof(INPUT));
    }

    // std::cout << "Receive data from [" << user << "], " << ip.type << " "
    //           << ip.mi.dwFlags << " " << ip.mi.dx << " " << ip.mi.dy
    //           << std::endl;
  }

#elif __APPLE__
  if (remote_action.type == ControlType::mouse) {
    CGEventRef mouse_event;
    CGEventType mouse_type;

    if (remote_action.m.flag == MouseFlag::left_down) {
      mouse_type = kCGEventLeftMouseDown;
    } else if (remote_action.m.flag == MouseFlag::left_up) {
      mouse_type = kCGEventLeftMouseUp;
    } else if (remote_action.m.flag == MouseFlag::right_down) {
      mouse_type = kCGEventRightMouseDown;
    } else if (remote_action.m.flag == MouseFlag::right_up) {
      mouse_type = kCGEventRightMouseUp;
    } else {
      mouse_type = kCGEventMouseMoved;
    }

    mouse_event = CGEventCreateMouseEvent(NULL, mouse_type,
                                          CGPointMake(mouse_pos_x, mouse_pos_y),
                                          kCGMouseButtonLeft);

    CGEventPost(kCGHIDEventTap, mouse_event);
    CFRelease(mouse_event);
  }
#endif
#endif
}

void ClientReceiveDataBuffer(const char *data, size_t size, const char *user_id,
                             size_t user_id_size) {
  std::string user(user_id, user_id_size);

  RemoteAction remote_action;
  memcpy(&remote_action, data, sizeof(remote_action));

  std::cout << "remote_action: " << remote_action.type << " "
            << remote_action.m.flag << " " << remote_action.m.x << " "
            << remote_action.m.y << std::endl;

  int mouse_pos_x = remote_action.m.x * screen_w / 1280;
  int mouse_pos_y = remote_action.m.y * screen_h / 720;

#ifdef _WIN32
  INPUT ip;

  if (remote_action.type == ControlType::mouse) {
    ip.type = INPUT_MOUSE;
    ip.mi.dx = mouse_pos_x;
    ip.mi.dy = mouse_pos_y;
    if (remote_action.m.flag == MouseFlag::left_down) {
      ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE;
    } else if (remote_action.m.flag == MouseFlag::left_up) {
      ip.mi.dwFlags = MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE;
    } else if (remote_action.m.flag == MouseFlag::right_down) {
      ip.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_ABSOLUTE;
    } else if (remote_action.m.flag == MouseFlag::right_up) {
      ip.mi.dwFlags = MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_ABSOLUTE;
    } else {
      ip.mi.dwFlags = MOUSEEVENTF_MOVE;
    }
    ip.mi.mouseData = 0;
    ip.mi.time = 0;

#if MOUSE_CONTROL
    // Set cursor pos
    SetCursorPos(ip.mi.dx, ip.mi.dy);
    // Send the press
    if (ip.mi.dwFlags != MOUSEEVENTF_MOVE) {
      SendInput(1, &ip, sizeof(INPUT));
    }
#endif
    // std::cout << "Receive data from [" << user << "], " << ip.type << " "
    //           << ip.mi.dwFlags << " " << ip.mi.dx << " " << ip.mi.dy
    //           << std::endl;
  }

#elif __APPLE__
  CGEventRef mouse_event;
  mouse_event = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved,
                                        CGPointMake(mouse_pos_x, mouse_pos_y),
                                        kCGMouseButtonLeft);
  CGEventPost(kCGHIDEventTap, mouse_event);
  CFRelease(mouse_event);

#endif
}

void ServerSignalStatus(SignalStatus status) {
  server_signal_status = status;
  if (SignalStatus::SignalConnecting == status) {
    server_signal_status_str = "ServerSignalConnecting";
  } else if (SignalStatus::SignalConnected == status) {
    server_signal_status_str = "ServerSignalConnected";
  } else if (SignalStatus::SignalFailed == status) {
    server_signal_status_str = "ServerSignalFailed";
  } else if (SignalStatus::SignalClosed == status) {
    server_signal_status_str = "ServerSignalClosed";
  } else if (SignalStatus::SignalReconnecting == status) {
    server_signal_status_str = "ServerSignalReconnecting";
  }
}

void ClientSignalStatus(SignalStatus status) {
  client_signal_status = status;
  if (SignalStatus::SignalConnecting == status) {
    client_signal_status_str = "ClientSignalConnecting";
  } else if (SignalStatus::SignalConnected == status) {
    client_signal_status_str = "ClientSignalConnected";
  } else if (SignalStatus::SignalFailed == status) {
    client_signal_status_str = "ClientSignalFailed";
  } else if (SignalStatus::SignalClosed == status) {
    client_signal_status_str = "ClientSignalClosed";
  } else if (SignalStatus::SignalReconnecting == status) {
    client_signal_status_str = "ClientSignalReconnecting";
  }
}

void ServerConnectionStatus(ConnectionStatus status) {
  server_connection_status = status;
  if (ConnectionStatus::Connecting == status) {
    server_connection_status_str = "ServerConnecting";
  } else if (ConnectionStatus::Connected == status) {
    server_connection_status_str = "ServerConnected";
  } else if (ConnectionStatus::Disconnected == status) {
    server_connection_status_str = "ServerDisconnected";
  } else if (ConnectionStatus::Failed == status) {
    server_connection_status_str = "ServerFailed";
  } else if (ConnectionStatus::Closed == status) {
    server_connection_status_str = "ServerClosed";
  } else if (ConnectionStatus::IncorrectPassword == status) {
    server_connection_status_str = "Incorrect password";
    if (connect_button_pressed) {
      connect_button_pressed = false;
      joined = false;
      connect_label = connect_button_pressed ? "Disconnect" : "Connect";
    }
  }
}

void ClientConnectionStatus(ConnectionStatus status) {
  client_connection_status = status;
  if (ConnectionStatus::Connecting == status) {
    client_connection_status_str = "ClientConnecting";
  } else if (ConnectionStatus::Connected == status) {
    client_connection_status_str = "ClientConnected";
  } else if (ConnectionStatus::Disconnected == status) {
    client_connection_status_str = "ClientDisconnected";
  } else if (ConnectionStatus::Failed == status) {
    client_connection_status_str = "ClientFailed";
  } else if (ConnectionStatus::Closed == status) {
    client_connection_status_str = "ClientClosed";
  } else if (ConnectionStatus::IncorrectPassword == status) {
    client_connection_status_str = "Incorrect password";
    if (connect_button_pressed) {
      connect_button_pressed = false;
      joined = false;
      connect_label = connect_button_pressed ? "Disconnect" : "Connect";
    }
  } else if (ConnectionStatus::NoSuchTransmissionId == status) {
    client_connection_status_str = "No such transmission id";
    if (connect_button_pressed) {
      connect_button_pressed = false;
      joined = false;
      connect_label = connect_button_pressed ? "Disconnect" : "Connect";
    }
  }
}

int initResampler() {
  // dst_filename = "res.pcm";

  // dst_file = fopen(dst_filename, "wb");
  // if (!dst_file) {
  //   fprintf(stderr, "Could not open destination file %s\n", dst_filename);
  //   exit(1);
  // }

  // 创建重采样器
  /* create resampler context */
  swr_ctx = swr_alloc();
  if (!swr_ctx) {
    fprintf(stderr, "Could not allocate resampler context\n");
    ret = AVERROR(ENOMEM);
    return -1;
  }

  // 设置重采样参数
  /* set options */
  // 输入参数
  av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
  av_opt_set_int(swr_ctx, "in_sample_rate", src_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);
  // 输出参数
  av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
  av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

  // 初始化重采样
  /* initialize the resampling context */
  if ((ret = swr_init(swr_ctx)) < 0) {
    fprintf(stderr, "Failed to initialize the resampling context\n");
    return -1;
  }

  /* allocate source and destination samples buffers */
  // 计算出输入源的通道数量
  src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
  // 给输入源分配内存空间
  ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize,
                                           src_nb_channels, src_nb_samples,
                                           src_sample_fmt, 0);
  if (ret < 0) {
    fprintf(stderr, "Could not allocate source samples\n");
    return -1;
  }

  // 计算输出采样数量
  max_dst_nb_samples = dst_nb_samples =
      av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

  /* buffer is going to be directly written to a rawaudio file, no alignment */
  dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
  // 分配输出缓存内存
  ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize,
                                           dst_nb_channels, dst_nb_samples,
                                           dst_sample_fmt, 0);
  if (ret < 0) {
    fprintf(stderr, "Could not allocate destination samples\n");
    return -1;
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
#elif __APPLE__
  std::string if_name = "en0";

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
          strcmp(if_name.c_str(), cursor->ifa_name) == 0) {
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
#elif __linux__
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    return "";
  }
  struct ifreq ifr;
  struct ifconf ifc;
  char buf[1024];
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
    close(sock);
    return "";
  }
  struct ifreq *it = ifc.ifc_req;
  const struct ifreq *const end = it + (ifc.ifc_len / sizeof(struct ifreq));
  for (; it != end; ++it) {
    std::strcpy(ifr.ifr_name, it->ifr_name);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
      continue;
    }
    if (ifr.ifr_flags & IFF_LOOPBACK) {
      continue;
    }
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
      continue;
    }
    std::string mac_address;
    for (int i = 0; i < 6; ++i) {
      len += sprintf(mac_addr + len, "%.2X", ifr.ifr_hwaddr.sa_data[i] & 0xff);
    }
    break;
  }
  close(sock);
#endif
  return mac_addr;
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

int main() {
  LOG_INFO("Remote desk");

  last_ts = static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count());

  initResampler();

  cd_cache_file = fopen("cache.cd", "r+");
  if (cd_cache_file) {
    fseek(cd_cache_file, 0, SEEK_SET);
    fread(&cd_cache.password, sizeof(cd_cache.password), 1, cd_cache_file);
    fclose(cd_cache_file);
    strncpy(input_password, cd_cache.password, sizeof(cd_cache.password));
  }

  std::thread rtc_thread([] {
    std::string default_cfg_path = "../../../../config/config.ini";
    std::ifstream f(default_cfg_path.c_str());

    Params server_params;
    server_params.cfg_path =
        f.good() ? "../../../../config/config.ini" : "config.ini";
    server_params.on_receive_video_buffer = ServerReceiveVideoBuffer;
    server_params.on_receive_audio_buffer = ServerReceiveAudioBuffer;
    server_params.on_receive_data_buffer = ServerReceiveDataBuffer;
    server_params.on_signal_status = ServerSignalStatus;
    server_params.on_connection_status = ServerConnectionStatus;

    Params client_params;
    client_params.cfg_path =
        f.good() ? "../../../../config/config.ini" : "config.ini";
    client_params.on_receive_video_buffer = ClientReceiveVideoBuffer;
    client_params.on_receive_audio_buffer = ClientReceiveAudioBuffer;
    client_params.on_receive_data_buffer = ClientReceiveDataBuffer;
    client_params.on_signal_status = ClientSignalStatus;
    client_params.on_connection_status = ClientConnectionStatus;

    std::string transmission_id = "000001";
    GetMac(mac_addr);

    peer_server = CreatePeer(&server_params);
    LOG_INFO("Create peer_server");
    std::string server_user_id = "S-" + std::string(GetMac(mac_addr));
    Init(peer_server, server_user_id.c_str());
    LOG_INFO("peer_server init finish");

    peer_client = CreatePeer(&client_params);
    LOG_INFO("Create peer_client");
    std::string client_user_id = "C-" + std::string(GetMac(mac_addr));
    Init(peer_client, client_user_id.c_str());
    LOG_INFO("peer_client init finish");

    {
      while (SignalStatus::SignalConnected != server_signal_status && !done) {
      }

      if (done) {
        return;
      }

      std::string user_id = "S-" + std::string(GetMac(mac_addr));
      is_create_connection =
          CreateConnection(peer_server, mac_addr, input_password) ? false
                                                                  : true;

      nv12_buffer = new char[NV12_BUFFER_SIZE];
#ifdef _WIN32
      screen_capture = new ScreenCaptureWgc();

      RECORD_DESKTOP_RECT rect;
      rect.left = 0;
      rect.top = 0;
      rect.right = GetSystemMetrics(SM_CXSCREEN);
      rect.bottom = GetSystemMetrics(SM_CYSCREEN);

      last_frame_time_ = std::chrono::high_resolution_clock::now();
      screen_capture->Init(
          rect, 60,
          [](unsigned char *data, int size, int width, int height) -> void {
            auto now_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration =
                now_time - last_frame_time_;
            auto tc = duration.count() * 1000;

            if (tc >= 0) {
              BGRAToNV12FFmpeg(data, width, height,
                               (unsigned char *)nv12_buffer);
              SendData(peer_server, DATA_TYPE::VIDEO, (const char *)nv12_buffer,
                       NV12_BUFFER_SIZE);
              // std::cout << "Send" << std::endl;
              last_frame_time_ = now_time;
            }
          });

      screen_capture->Start();

#elif __linux__
      screen_capture = new ScreenCaptureX11();

      RECORD_DESKTOP_RECT rect;
      rect.left = 0;
      rect.top = 0;
      rect.right = 0;
      rect.bottom = 0;

      last_frame_time_ = std::chrono::high_resolution_clock::now();
      screen_capture->Init(
          rect, 60,
          [](unsigned char *data, int size, int width, int height) -> void {
            auto now_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration =
                now_time - last_frame_time_;
            auto tc = duration.count() * 1000;

            if (tc >= 0) {
              SendData(peer_server, DATA_TYPE::VIDEO, (const char *)data,
                       NV12_BUFFER_SIZE);
              last_frame_time_ = now_time;
            }
          });
      screen_capture->Start();
#elif __APPLE__
      screen_capture = new ScreenCaptureAvf();

      RECORD_DESKTOP_RECT rect;
      rect.left = 0;
      rect.top = 0;
      rect.right = 0;
      rect.bottom = 0;

      last_frame_time_ = std::chrono::high_resolution_clock::now();
      screen_capture->Init(
          rect, 60,
          [](unsigned char *data, int size, int width, int height) -> void {
            auto now_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration =
                now_time - last_frame_time_;
            auto tc = duration.count() * 1000;

            if (tc >= 0) {
              SendData(peer_server, DATA_TYPE::VIDEO, (const char *)data,
                       NV12_BUFFER_SIZE);
              last_frame_time_ = now_time;
            }
          });
      screen_capture->Start();
#endif
    }
  });

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER |
               SDL_INIT_GAMECONTROLLER) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

  // Create window with SDL_Renderer graphics context
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window = SDL_CreateWindow("Remote Desk", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, window_w, window_h,
                            window_flags);

  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);
  screen_w = DM.w;
  screen_h = DM.h;

  sdlRenderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (sdlRenderer == nullptr) {
    SDL_Log("Error creating SDL_Renderer!");
    return 0;
  }

  Uint32 pixformat = 0;
  pixformat = SDL_PIXELFORMAT_NV12;

  sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat,
                                 SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);

  // Audio
  SDL_AudioSpec want_in, have_in, want_out, have_out;
  SDL_zero(want_in);
  want_in.freq = 48000;
  want_in.format = AUDIO_S16LSB;
  want_in.channels = 1;
  want_in.samples = 480;
  want_in.callback = SdlCaptureAudioIn;

  input_dev = SDL_OpenAudioDevice(NULL, 1, &want_in, &have_in, 0);
  if (input_dev == 0) {
    SDL_Log("Failed to open input: %s", SDL_GetError());
    return 1;
  }

  SDL_zero(want_out);
  want_out.freq = 48000;
  want_out.format = AUDIO_S16LSB;
  want_out.channels = 1;
  // want_out.silence = 0;
  want_out.samples = 480;
  want_out.callback = NULL;

  output_dev = SDL_OpenAudioDevice(NULL, 0, &want_out, &have_out, 0);
  if (output_dev == 0) {
    SDL_Log("Failed to open input: %s", SDL_GetError());
    return 1;
  }

  SDL_PauseAudioDevice(input_dev, 0);
  SDL_PauseAudioDevice(output_dev, 0);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, sdlRenderer);
  ImGui_ImplSDLRenderer2_Init(sdlRenderer);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Main loop
  while (!done) {
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    {
      static float f = 0.0f;
      static int counter = 0;

      const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
      ImGui::SetNextWindowSize(ImVec2(190, 200));

      ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoResize);

      {
        ImGui::Text(" LOCAL ID:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95);
        ImGui::InputText("##local_id", mac_addr, IM_ARRAYSIZE(mac_addr),
                         ImGuiInputTextFlags_CharsUppercase);

        ImGui::Text(" PASSWORD:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95);

        char input_password_tmp[7] = "";
        strncpy(input_password_tmp, input_password, sizeof(input_password));

        ImGui::InputTextWithHint("##server_pwd", "max 6 chars", input_password,
                                 IM_ARRAYSIZE(input_password),
                                 ImGuiInputTextFlags_CharsNoBlank);
        if (strcmp(input_password_tmp, input_password)) {
          cd_cache_file = fopen("cache.cd", "w+");
          if (cd_cache_file) {
            fseek(cd_cache_file, 0, SEEK_SET);
            strncpy(cd_cache.password, input_password, sizeof(input_password));
            fwrite(&cd_cache.password, sizeof(cd_cache.password), 1,
                   cd_cache_file);
            fclose(cd_cache_file);
          }
          LeaveConnection(peer_server);
          CreateConnection(peer_server, mac_addr, input_password);
        }

        ImGui::Spacing();

        ImGui::Separator();

        ImGui::Spacing();
        {
          {
            static char remote_id[20] = "";
            // if (strcmp(remote_id, "") == 0) {
            //   strcpy(remote_id, GetMac(mac_addr).c_str());
            // }
            ImGui::Text("REMOTE ID:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(95);
            ImGui::InputTextWithHint("##remote_id", mac_addr, remote_id,
                                     IM_ARRAYSIZE(remote_id),
                                     ImGuiInputTextFlags_CharsNoBlank);

            ImGui::Spacing();

            ImGui::Text(" PASSWORD:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(95);
            static char client_password[20] = "";
            ImGui::InputTextWithHint("##client_pwd", "max 6 chars",
                                     client_password,
                                     IM_ARRAYSIZE(client_password),
                                     ImGuiInputTextFlags_CharsNoBlank);

            if (ImGui::Button(connect_label)) {
              int ret = -1;
              if ("ClientSignalConnected" == client_signal_status_str) {
                if (strcmp(connect_label, "Connect") == 0 && !joined) {
                  std::string user_id = "C-" + std::string(GetMac(mac_addr));
                  ret = JoinConnection(peer_client, remote_id, client_password);
                  if (0 == ret) {
                    joined = true;
                  }
                } else if (strcmp(connect_label, "Disconnect") == 0 && joined) {
                  ret = LeaveConnection(peer_client);
                  memset(audio_buffer, 0, 960);
                  if (0 == ret) {
                    joined = false;
                    received_frame = false;
                  }
                }

                if (0 == ret) {
                  connect_button_pressed = !connect_button_pressed;
                  connect_label =
                      connect_button_pressed ? "Disconnect" : "Connect";
                }
              }
            }
          }
        }
      }

      ImGui::Spacing();

      ImGui::Separator();

      ImGui::Spacing();

      {
        if (ImGui::Button("Resize Window")) {
          SDL_GetWindowSize(window, &window_w, &window_h);

          if (window_h != window_w * 9 / 16) {
            window_w = window_h * 16 / 9;
          }

          SDL_SetWindowSize(window, window_w, window_h);
        }
      }

      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    SDL_RenderSetScale(sdlRenderer, io.DisplayFramebufferScale.x,
                       io.DisplayFramebufferScale.y);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
        done = true;
      } else if (event.type == SDL_WINDOWEVENT &&
                 event.window.event == SDL_WINDOWEVENT_RESIZED) {
        SDL_GetWindowSize(window, &window_w, &window_h);
      } else if (event.type == SDL_WINDOWEVENT &&
                 event.window.event == SDL_WINDOWEVENT_CLOSE &&
                 event.window.windowID == SDL_GetWindowID(window)) {
        done = true;
      } else if (event.type == REFRESH_EVENT) {
        sdlRect.x = 0;
        sdlRect.y = 0;
        sdlRect.w = window_w;
        sdlRect.h = window_h;

        SDL_UpdateTexture(sdlTexture, NULL, dst_buffer, pixel_w);
      } else {
        if (joined) {
          ProcessMouseKeyEven(event);
        }
      }
    }

    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);

    if (!joined || !received_frame) {
      SDL_RenderClear(sdlRenderer);
      SDL_SetRenderDrawColor(
          sdlRenderer, (Uint8)(clear_color.x * 0), (Uint8)(clear_color.y * 0),
          (Uint8)(clear_color.z * 0), (Uint8)(clear_color.w * 0));
    }

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(sdlRenderer);

    frame_count++;
    end_time = SDL_GetTicks();
    elapsed_time = end_time - start_time;
    if (elapsed_time >= 1000) {
      fps = frame_count / (elapsed_time / 1000);
      frame_count = 0;
      window_title = "Remote Desk Client FPS [" + std::to_string(fps) +
                     "] status [" + server_signal_status_str + "|" +
                     client_signal_status_str + "|" +
                     server_connection_status_str + "|" +
                     client_connection_status_str + "]";
      // For MacOS, UI frameworks can only be called from the main thread
      SDL_SetWindowTitle(window, window_title.c_str());
      start_time = end_time;
    }
  }

  // Cleanup

  if (is_create_connection) {
    LeaveConnection(peer_server);
  }

  if (joined) {
    LeaveConnection(peer_client);
  }

  rtc_thread.join();
  SDL_CloseAudioDevice(output_dev);
  SDL_CloseAudioDevice(input_dev);

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(window);

  SDL_CloseAudio();
  SDL_Quit();

  return 0;
}