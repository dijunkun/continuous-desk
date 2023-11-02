#include "screen_capture_x11.h"

#include <iostream>

#define NV12_BUFFER_SIZE 1280 * 720 * 3 / 2
unsigned char nv12_buffer_[NV12_BUFFER_SIZE];

FILE *file = nullptr;

ScreenCaptureX11::ScreenCaptureX11() {
  file = fopen("nv12.yuv", "w+b");
  if (!file) {
    printf("Fail to open nv12.yuv\n");
  }
}

ScreenCaptureX11::~ScreenCaptureX11() {
  if (capture_thread_->joinable()) {
    capture_thread_->join();
  }
}

int ScreenCaptureX11::Init(const RECORD_DESKTOP_RECT &rect, const int fps,
                           cb_desktop_data cb) {
  if (cb) {
    _on_data = cb;
  }

  pFormatCtx_ = avformat_alloc_context();

  avdevice_register_all();

  // grabbing frame rate
  av_dict_set(&options_, "framerate", "30", 0);
  // Make the grabbed area follow the mouse
  // av_dict_set(&options_, "follow_mouse", "centered", 0);
  // Video frame size. The default is to capture the full screen
  av_dict_set(&options_, "video_size", "1280x720", 0);
  ifmt_ = (AVInputFormat *)av_find_input_format("x11grab");
  if (!ifmt_) {
    printf("Couldn't find_input_format\n");
  }

  // Grab at position 10,20
  if (avformat_open_input(&pFormatCtx_, ":0.0", ifmt_, &options_) != 0) {
    printf("Couldn't open input stream.\n");
    return -1;
  }

  if (avformat_find_stream_info(pFormatCtx_, NULL) < 0) {
    printf("Couldn't find stream information.\n");
    return -1;
  }

  videoindex_ = -1;
  for (i_ = 0; i_ < pFormatCtx_->nb_streams; i_++)
    if (pFormatCtx_->streams[i_]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoindex_ = i_;
      break;
    }
  if (videoindex_ == -1) {
    printf("Didn't find a video stream.\n");
    return -1;
  }

  pCodecParam_ = pFormatCtx_->streams[videoindex_]->codecpar;

  pCodecCtx_ = avcodec_alloc_context3(NULL);
  avcodec_parameters_to_context(pCodecCtx_, pCodecParam_);

  pCodec_ = const_cast<AVCodec *>(avcodec_find_decoder(pCodecCtx_->codec_id));
  if (pCodec_ == NULL) {
    printf("Codec not found.\n");
    return -1;
  }
  if (avcodec_open2(pCodecCtx_, pCodec_, NULL) < 0) {
    printf("Could not open codec.\n");
    return -1;
  }

  pFrame_ = av_frame_alloc();
  pFrameNV12_ = av_frame_alloc();

  const int pixel_w = 1280, pixel_h = 720;
  int screen_w = 1280, screen_h = 720;

  screen_w = 1280;
  screen_h = 720;

  packet_ = (AVPacket *)av_malloc(sizeof(AVPacket));

  img_convert_ctx_ =
      sws_getContext(pCodecCtx_->width, pCodecCtx_->height, pCodecCtx_->pix_fmt,
                     pCodecCtx_->width, pCodecCtx_->height, AV_PIX_FMT_NV12,
                     SWS_BICUBIC, NULL, NULL, NULL);

  return 0;
}

int ScreenCaptureX11::Start() {
  capture_thread_.reset(new std::thread([this]() {
    while (1) {
      printf("00000\n");
      if (av_read_frame(pFormatCtx_, packet_) >= 0) {
        printf("111111444444\n");
        if (packet_->stream_index == videoindex_) {
          printf("11111155555\n");
          avcodec_send_packet(pCodecCtx_, packet_);
          got_picture_ = avcodec_receive_frame(pCodecCtx_, pFrame_);
          printf("33333333\n");
          if (!got_picture_) {
            printf("44444444444\n");

            av_image_fill_arrays(pFrameNV12_->data, pFrameNV12_->linesize,
                                 nv12_buffer_, AV_PIX_FMT_NV12, pFrame_->width,
                                 pFrame_->height, 1);

            sws_scale(img_convert_ctx_, pFrame_->data, pFrame_->linesize, 0,
                      pFrame_->height, pFrameNV12_->data,
                      pFrameNV12_->linesize);

            fwrite(nv12_buffer_, 1, pFrame_->width * pFrame_->height * 3 / 2,
                   file);

            _on_data((unsigned char *)nv12_buffer_,
                     pFrame_->width * pFrame_->height * 3 / 2, pFrame_->width,
                     pFrame_->height);
          }
        }
      }
    }
  }));

  return 0;
}

int ScreenCaptureX11::Pause() { return 0; }

int ScreenCaptureX11::Resume() { return 0; }

int ScreenCaptureX11::Stop() { return 0; }

void ScreenCaptureX11::OnFrame() {}

void ScreenCaptureX11::CleanUp() {}
