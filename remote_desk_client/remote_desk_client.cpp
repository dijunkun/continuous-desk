#ifdef _WIN32
#include <Winsock2.h>
#include <iphlpapi.h>
#elif __APPLE__
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

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "x.h"

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

int screen_w = 1280, screen_h = 720;
const int pixel_w = 1280, pixel_h = 720;

unsigned char dst_buffer[pixel_w * pixel_h * 3 / 2];
SDL_Texture *sdlTexture = nullptr;
SDL_Renderer *sdlRenderer = nullptr;
SDL_Rect sdlRect;
SDL_Window *screen;

uint32_t start_time, end_time, elapsed_time;
uint32_t frame_count = 0;
int fps = 0;
std::string window_title = "Remote Desk Client";

// Refresh Event
#define REFRESH_EVENT (SDL_USEREVENT + 1)
#define QUIT_EVENT (SDL_USEREVENT + 2)

int thread_exit = 0;
PeerPtr *peer = nullptr;

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

inline void FreshVideo() {
  sdlRect.x = 0;
  sdlRect.y = 0;
  sdlRect.w = screen_w;
  sdlRect.h = screen_h;

  SDL_UpdateTexture(sdlTexture, NULL, dst_buffer, pixel_w);
  SDL_RenderClear(sdlRenderer);
  SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
  SDL_RenderPresent(sdlRenderer);

  frame_count++;
  end_time = SDL_GetTicks();
  elapsed_time = end_time - start_time;
  if (elapsed_time >= 1000) {
    fps = frame_count / (elapsed_time / 1000);
    frame_count = 0;
    window_title = "Remote Desk Client FPS [" + std::to_string(fps) + "]";
    // For MacOS, UI frameworks can only be called from the main thread
    SDL_SetWindowTitle(screen, window_title.c_str());
    start_time = end_time;
  }
}

inline int ProcessMouseKeyEven(SDL_Event &ev) {
  float ratio = 1280.0 / screen_w;

  RemoteAction remote_action;

  if (SDL_KEYDOWN == ev.type)  // SDL_KEYUP
  {
    printf("SDLK_DOWN: %d\n", SDL_KeyCode(ev.key.keysym.sym));
    if (SDLK_DOWN == ev.key.keysym.sym) {
      printf("SDLK_DOWN  \n");

    } else if (SDLK_UP == ev.key.keysym.sym) {
      printf("SDLK_UP  \n");

    } else if (SDLK_LEFT == ev.key.keysym.sym) {
      printf("SDLK_LEFT  \n");

    } else if (SDLK_RIGHT == ev.key.keysym.sym) {
      printf("SDLK_RIGHT  \n");
    }
  } else if (SDL_MOUSEBUTTONDOWN == ev.type) {
    if (SDL_BUTTON_LEFT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      printf("SDL_MOUSEBUTTONDOWN x, y %d %d  \n", px, py);

      remote_action.type = ControlType::mouse;
      remote_action.m.flag = MouseFlag::left_down;
      remote_action.m.x = ev.button.x * ratio;
      remote_action.m.y = ev.button.y * ratio;

    } else if (SDL_BUTTON_RIGHT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      printf("SDL_BUTTON_RIGHT x, y %d %d  \n", px, py);

      remote_action.type = ControlType::mouse;
      remote_action.m.flag = MouseFlag::right_down;
      remote_action.m.x = ev.button.x * ratio;
      remote_action.m.y = ev.button.y * ratio;
    }
  } else if (SDL_MOUSEBUTTONUP == ev.type) {
    if (SDL_BUTTON_LEFT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      printf("SDL_MOUSEBUTTONUP x, y %d %d  \n", px, py);

      remote_action.type = ControlType::mouse;
      remote_action.m.flag = MouseFlag::left_up;
      remote_action.m.x = ev.button.x * ratio;
      remote_action.m.y = ev.button.y * ratio;

    } else if (SDL_BUTTON_RIGHT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      printf("SDL_MOUSEBUTTONUP x, y %d %d  \n", px, py);

      remote_action.type = ControlType::mouse;
      remote_action.m.flag = MouseFlag::right_up;
      remote_action.m.x = ev.button.x * ratio;
      remote_action.m.y = ev.button.y * ratio;
    }
  } else if (SDL_MOUSEMOTION == ev.type) {
    int px = ev.motion.x;
    int py = ev.motion.y;

    printf("SDL_MOUSEMOTION x, y %d %d  \n", px, py);

    remote_action.type = ControlType::mouse;
    remote_action.m.flag = MouseFlag::move;
    remote_action.m.x = ev.button.x * ratio;
    remote_action.m.y = ev.button.y * ratio;
  } else if (SDL_QUIT == ev.type) {
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
    printf("SDL_QUIT\n");
    return 0;
  }

  SendData(peer, DATA_TYPE::DATA, (const char *)&remote_action,
           sizeof(remote_action));
  // std::cout << remote_action.type << " " << remote_action.type << " "
  //           << remote_action.px << " " << remote_action.py << std::endl;

  return 0;
}

void ReceiveVideoBuffer(const char *data, size_t size, const char *user_id,
                        size_t user_id_size) {
  // std::cout << "Receive: [" << user_id << "] " << std::endl;
  memcpy(dst_buffer, data, size);

  SDL_Event event;
  event.type = REFRESH_EVENT;
  SDL_PushEvent(&event);
}

void ReceiveAudioBuffer(const char *data, size_t size, const char *user_id,
                        size_t user_id_size) {
  std::cout << "Receive audio, size " << size << ", user [" << user_id << "] "
            << std::endl;
}

void ReceiveDataBuffer(const char *data, size_t size, const char *user_id,
                       size_t user_id_size) {
  std::cout << "Receive data, size " << size << ", user [" << user_id << "] "
            << std::endl;
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

int main() {
  std::string default_cfg_path = "../../../../config/config.ini";
  std::ifstream f(default_cfg_path.c_str());

  Params params;
  params.cfg_path = f.good() ? "../../../../config/config.ini" : "config.ini";
  params.on_receive_video_buffer = ReceiveVideoBuffer;
  params.on_receive_audio_buffer = ReceiveAudioBuffer;
  params.on_receive_data_buffer = ReceiveDataBuffer;

  std::string transmission_id = "000001";
  char mac_addr[10];
  std::string user_id = "C-" + std::string(GetMac(mac_addr));

  peer = CreatePeer(&params);
  JoinConnection(peer, transmission_id.c_str(), user_id.c_str());

  if (SDL_Init(SDL_INIT_VIDEO)) {
    printf("Could not initialize SDL - %s\n", SDL_GetError());
    return -1;
  }

  screen = SDL_CreateWindow("RTS Receiver", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h,
                            SDL_WINDOW_RESIZABLE);

  SDL_SetWindowTitle(screen, window_title.data());

  if (!screen) {
    printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
    return -1;
  }
  sdlRenderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);

  Uint32 pixformat = 0;
  pixformat = SDL_PIXELFORMAT_NV12;

  sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat,
                                 SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);

  SDL_Event event;
  start_time = SDL_GetTicks();
  while (1) {
    // Wait
    // SDL_WaitEvent(&event);
    while (SDL_PollEvent(&event)) {
      if (event.type == REFRESH_EVENT) {
        FreshVideo();
      } else if (event.type == SDL_WINDOWEVENT) {
        int new_screen_w = 0;
        int new_screen_h = 0;
        SDL_GetWindowSize(screen, &new_screen_w, &new_screen_h);

        if (new_screen_w != screen_w) {
          screen_w = new_screen_w;
          screen_h = new_screen_w * 9 / 16;
        } else if (new_screen_h != screen_h) {
          screen_w = new_screen_h * 16 / 9;
          screen_h = new_screen_h;
        }
        SDL_SetWindowSize(screen, screen_w, screen_h);
        printf("Resize windows: %dx%d\n", screen_w, screen_h);
      } else if (event.type == SDL_QUIT) {
        return 0;
      } else {
        ProcessMouseKeyEven(event);
      }
    }
  }

  return 0;
}
