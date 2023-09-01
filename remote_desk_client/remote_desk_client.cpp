#ifdef _WIN32
#include <Winsock2.h>
#include <iphlpapi.h>
#endif

#include <chrono>
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
    window_title = "Remote Desk Client [FPS " + std::to_string(fps) + "]";
    SDL_SetWindowTitle(screen, window_title.data());
    start_time = end_time;
  }
}

inline int ProcessMouseKeyEven(SDL_Event &ev) {
  if (SDL_KEYDOWN == ev.type)  // SDL_KEYUP
  {
    if (SDLK_DOWN == ev.key.keysym.sym) {
      printf("SDLK_DOWN ...............\n");

    } else if (SDLK_UP == ev.key.keysym.sym) {
      printf("SDLK_UP ...............\n");

    } else if (SDLK_LEFT == ev.key.keysym.sym) {
      printf("SDLK_LEFT ...............\n");

    } else if (SDLK_RIGHT == ev.key.keysym.sym) {
      printf("SDLK_RIGHT ...............\n");
    }
  } else if (SDL_MOUSEBUTTONDOWN == ev.type) {
    if (SDL_BUTTON_LEFT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      printf("SDL_MOUSEBUTTONDOWN x, y %d %d ...............\n", px, py);

    } else if (SDL_BUTTON_RIGHT == ev.button.button) {
      int px = ev.button.x;
      int py = ev.button.y;
      printf("SDL_BUTTON_RIGHT x, y %d %d ...............\n", px, py);
    }
  } else if (SDL_MOUSEMOTION == ev.type) {
    int px = ev.motion.x;
    int py = ev.motion.y;

    printf("SDL_MOUSEMOTION x, y %d %d ...............\n", px, py);
  } else if (SDL_QUIT == ev.type) {
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
    printf("SDL_QUIT\n");
    return 0;
  }

  return 0;
}

void GuestReceiveBuffer(const char *data, size_t size, const char *user_id,
                        size_t user_id_size) {
  memcpy(dst_buffer, data, size);

  SDL_Event event;
  event.type = REFRESH_EVENT;
  SDL_PushEvent(&event);
}

std::string GetMac() {
  IP_ADAPTER_INFO adapterInfo[16];
  DWORD bufferSize = sizeof(adapterInfo);
  char mac[10];
  int len = 0;

  DWORD result = GetAdaptersInfo(adapterInfo, &bufferSize);
  if (result == ERROR_SUCCESS) {
    PIP_ADAPTER_INFO adapter = adapterInfo;
    while (adapter) {
      for (UINT i = 0; i < adapter->AddressLength; i++) {
        len += sprintf(mac + len, "%.2X", adapter->Address[i]);
      }
      break;
    }
  }

  return mac;
}

int main() {
  Params params;
  params.cfg_path = "config.ini";
  params.on_receive_buffer = GuestReceiveBuffer;

  std::string transmission_id = "000000";
  std::string user_id = GetMac();

  PeerPtr *peer = CreatePeer(&params);
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
        SDL_GetWindowSize(screen, &screen_w, &screen_h);
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
