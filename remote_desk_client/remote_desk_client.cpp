

#include <stdio.h>

#include <chrono>
#include <iostream>
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

// Refresh Event
#define REFRESH_EVENT (SDL_USEREVENT + 1)
#define QUIT_EVENT (SDL_USEREVENT + 2)

int thread_exit = 0;

int refresh_video(void *opaque) {
  SDL_Event event;
  while (thread_exit == 0) {
    event.type = REFRESH_EVENT;
    SDL_PushEvent(&event);
    SDL_Delay(10);
  }

  event.type = QUIT_EVENT;
  SDL_PushEvent(&event);

  return 0;
}

void GuestReceiveBuffer(const char *data, size_t size, const char *user_id,
                        size_t user_id_size) {
  std::cout << "Receive size: " << size << std::endl;
  memcpy(dst_buffer, data, size);
}

int main() {
  Params params;
  params.cfg_path = "config.ini";
  params.on_receive_buffer = GuestReceiveBuffer;

  std::string transmission_id = "000000";
  std::string user_id = "Client";

  PeerPtr *peer = CreatePeer(&params);
  JoinConnection(peer, transmission_id.c_str(), user_id.c_str());

  if (SDL_Init(SDL_INIT_VIDEO)) {
    printf("Could not initialize SDL - %s\n", SDL_GetError());
    return -1;
  }

  SDL_Window *screen;
  screen = SDL_CreateWindow("RTS Receiver", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h,
                            SDL_WINDOW_RESIZABLE);
  if (!screen) {
    printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
    return -1;
  }
  sdlRenderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);

  Uint32 pixformat = 0;
  pixformat = SDL_PIXELFORMAT_NV12;

  sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat,
                                 SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);

  SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
  SDL_Event event;
  while (1) {
    // Wait
    SDL_WaitEvent(&event);
    if (event.type == REFRESH_EVENT) {
      sdlRect.x = 0;
      sdlRect.y = 0;
      sdlRect.w = screen_w;
      sdlRect.h = screen_h;

      SDL_UpdateTexture(sdlTexture, NULL, dst_buffer, pixel_w);
      SDL_RenderClear(sdlRenderer);
      SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
      SDL_RenderPresent(sdlRenderer);
    } else if (event.type == SDL_WINDOWEVENT) {
      // If Resize
      SDL_GetWindowSize(screen, &screen_w, &screen_h);
      printf("Resize windows: %dx%d\n", screen_w, screen_h);
    } else if (event.type == SDL_QUIT) {
      break;
    }
  }

  return 0;
}
