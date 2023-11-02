#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "SDL2/SDL.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C" {
#endif
#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
};
#endif
#endif

#include <chrono>

// Output YUV420P
#define OUTPUT_YUV420P 0
//'1' Use Dshow
//'0' Use GDIgrab
#define USE_DSHOW 0

// Refresh Event
#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

#define NV12_BUFFER_SIZE 1280 * 720 * 3 / 2

int thread_exit = 0;
SDL_Texture *sdlTexture = nullptr;
SDL_Renderer *sdlRenderer = nullptr;
SDL_Rect sdlRect;
unsigned char nv12_buffer[NV12_BUFFER_SIZE];
std::chrono::_V2::system_clock::time_point last_frame_time;
const int pixel_w = 1280, pixel_h = 720;
int screen_w = 1280, screen_h = 720;
bool done = false;

int YUV420ToNV12FFmpeg(unsigned char *src_buffer, int width, int height,
                       unsigned char *des_buffer) {
  AVFrame *Input_pFrame = av_frame_alloc();
  AVFrame *Output_pFrame = av_frame_alloc();
  struct SwsContext *img_convert_ctx = sws_getContext(
      width, height, AV_PIX_FMT_NV12, width, height, AV_PIX_FMT_YUV420P,
      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  av_image_fill_arrays(Input_pFrame->data, Input_pFrame->linesize, src_buffer,
                       AV_PIX_FMT_NV12, width, height, 1);
  av_image_fill_arrays(Output_pFrame->data, Output_pFrame->linesize, des_buffer,
                       AV_PIX_FMT_YUV420P, width, height, 1);

  sws_scale(img_convert_ctx, (uint8_t const **)Input_pFrame->data,
            Input_pFrame->linesize, 0, height, Output_pFrame->data,
            Output_pFrame->linesize);

  if (Input_pFrame) av_free(Input_pFrame);
  if (Output_pFrame) av_free(Output_pFrame);
  if (img_convert_ctx) sws_freeContext(img_convert_ctx);

  return 0;
}

int sfp_refresh_thread(void *opaque) {
  thread_exit = 0;
  while (!thread_exit) {
    SDL_Event event;
    event.type = SFM_REFRESH_EVENT;
    SDL_PushEvent(&event);
    SDL_Delay(30);
    printf("sfp_refresh_thread\n");
  }
  thread_exit = 0;
  // Break
  SDL_Event event;
  event.type = SFM_BREAK_EVENT;
  SDL_PushEvent(&event);
  printf("exit sfp_refresh_thread\n");

  return 0;
}

int main(int argc, char *argv[]) {
  AVFormatContext *pFormatCtx;
  int i, videoindex;
  AVCodecContext *pCodecCtx;
  AVCodec *pCodec;
  AVCodecParameters *pCodecParam;

  // avformat_network_init();
  pFormatCtx = avformat_alloc_context();

  // Open File
  char filepath[] = "out.h264";
  // avformat_open_input(&pFormatCtx, filepath, NULL, NULL);

  // Register Device
  avdevice_register_all();
  // Windows

  // Linux
  AVDictionary *options = NULL;
  // Set some options
  // grabbing frame rate
  av_dict_set(&options, "framerate", "30", 0);
  // Make the grabbed area follow the mouse
  // av_dict_set(&options, "follow_mouse", "centered", 0);
  // Video frame size. The default is to capture the full screen
  av_dict_set(&options, "video_size", "1280x720", 0);
  AVInputFormat *ifmt = (AVInputFormat *)av_find_input_format("x11grab");
  if (!ifmt) {
    printf("Couldn't find_input_format\n");
  }

  // Grab at position 10,20
  if (avformat_open_input(&pFormatCtx, ":0.0", ifmt, &options) != 0) {
    printf("Couldn't open input stream.\n");
    return -1;
  }

  if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    printf("Couldn't find stream information.\n");
    return -1;
  }

  videoindex = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++)
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoindex = i;
      break;
    }
  if (videoindex == -1) {
    printf("Didn't find a video stream.\n");
    return -1;
  }

  pCodecParam = pFormatCtx->streams[videoindex]->codecpar;

  pCodecCtx = avcodec_alloc_context3(NULL);
  avcodec_parameters_to_context(pCodecCtx, pCodecParam);

  pCodec = const_cast<AVCodec *>(avcodec_find_decoder(pCodecCtx->codec_id));
  if (pCodec == NULL) {
    printf("Codec not found.\n");
    return -1;
  }
  if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
    printf("Could not open codec.\n");
    return -1;
  }
  AVFrame *pFrame, *pFrameYUV, *pFrameNV12;
  pFrame = av_frame_alloc();
  pFrameYUV = av_frame_alloc();
  pFrameNV12 = av_frame_alloc();
  // unsigned char *out_buffer=(unsigned char
  // *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
  // pCodecCtx->height)); avpicture_fill((AVPicture *)pFrameYUV, out_buffer,
  // AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
  // SDL----------------------------
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    printf("Could not initialize SDL - %s\n", SDL_GetError());
    return -1;
  }

  // const SDL_VideoInfo *vi = SDL_GetVideoInfo();
  // Half of the Desktop's width and height.
  screen_w = 1280;
  screen_h = 720;
  // SDL_Surface *screen;
  // screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);
  SDL_Window *screen;
  screen = SDL_CreateWindow("Linux Capture", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h,
                            SDL_WINDOW_RESIZABLE);

  if (!screen) {
    printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
    return -1;
  }
  // SDL_Overlay *bmp;
  // bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,
  //                            SDL_YV12_OVERLAY, screen);

  sdlRenderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);

  Uint32 pixformat = 0;
  pixformat = SDL_PIXELFORMAT_NV12;

  SDL_Texture *sdlTexture = nullptr;
  sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat,
                                 SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);

  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = screen_w;
  rect.h = screen_h;
  // SDL End------------------------
  int ret, got_picture;

  AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

  struct SwsContext *img_convert_ctx;
  img_convert_ctx = sws_getContext(
      pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
      pCodecCtx->height, AV_PIX_FMT_NV12, SWS_BICUBIC, NULL, NULL, NULL);
  //------------------------------
  SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);
  //
  // SDL_WM_SetCaption("Simplest FFmpeg Grab Desktop", NULL);
  // Event Loop
  SDL_Event event;

  last_frame_time = std::chrono::high_resolution_clock::now();

  for (;;) {
    // Wait
    SDL_WaitEvent(&event);

    if (1) {
      //------------------------------
      if (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == videoindex) {
          avcodec_send_packet(pCodecCtx, packet);
          got_picture = avcodec_receive_frame(pCodecCtx, pFrame);
          // ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,
          // packet);
          if (ret < 0) {
            printf("Decode Error.\n");
            return -1;
          }
          printf("xxxxxxxxxxxxxxxxxxx\n");
          if (!got_picture) {
            auto now_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = now_time - last_frame_time;
            auto tc = duration.count() * 1000;
            printf("duration: %f\n", tc);
            last_frame_time = now_time;

            av_image_fill_arrays(pFrameNV12->data, pFrameNV12->linesize,
                                 nv12_buffer, AV_PIX_FMT_NV12, pFrame->width,
                                 pFrame->height, 1);

            sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
                      pFrame->height, pFrameNV12->data, pFrameNV12->linesize);

            SDL_UpdateTexture(sdlTexture, NULL, nv12_buffer, pixel_w);

            // FIX: If window is resize
            sdlRect.x = 0;
            sdlRect.y = 0;
            sdlRect.w = screen_w;
            sdlRect.h = screen_h;

            SDL_RenderClear(sdlRenderer);
            SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
            SDL_RenderPresent(sdlRenderer);
          }
        }
        // av_free_packet(packet);
      } else {
        // Exit Thread
        // thread_exit = 1;
        // printf("No frame read\n");
      }
    } else if (event.type == SDL_QUIT) {
      printf("SDL_QUIT\n");
      thread_exit = 1;
    } else if (event.type == SFM_BREAK_EVENT) {
      break;
    }
  }

  sws_freeContext(img_convert_ctx);

#if OUTPUT_YUV420P
  fclose(fp_yuv);
#endif

  SDL_Quit();

  // av_free(out_buffer);
  av_frame_free(&pFrameNV12);
  av_free(pFrameYUV);
  avcodec_close(pCodecCtx);
  avformat_close_input(&pFormatCtx);

  getchar();

  return 0;
}