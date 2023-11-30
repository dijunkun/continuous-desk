#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

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

static SDL_AudioDeviceID input_dev;
static SDL_AudioDeviceID output_dev;

static Uint8 *buffer = 0;
static int in_pos = 0;
static int out_pos = 0;

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
static unsigned char audio_buffer[960 * 3];
static int audio_len = 0;

// 输出文件
const char *dst_filename = NULL;  // 保存输出的pcm到本地，然后播放验证
FILE *dst_file;

int dst_bufsize;
const char *fmt;

// 重采样实例
struct SwrContext *swr_ctx;

double t;
int ret;

char *out = "audio_old.pcm";
FILE *outfile = fopen(out, "wb+");

void cb_in(void *userdata, Uint8 *stream, int len) {
  // If len < 4, the printf below will probably segfault
  // SDL_QueueAudio(output_dev, stream, len);

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
  printf("t:%f in:%d out:%d %d\n", t, src_nb_samples, ret, len);

  memcpy(audio_buffer, dst_data[0], len);
  // SDL_QueueAudio(output_dev, dst_data[0], len);
  audio_len = len;
}

void cb_out(void *userdata, Uint8 *stream, int len) {
  // If len < 4, the printf below will probably segfault
  printf("cb_out len = %d\n", len);
  SDL_memset(stream, 0, len);
  if (audio_len == 0) return;
  len = (len > audio_len ? audio_len : len);
  SDL_MixAudioFormat(stream, audio_buffer, AUDIO_S16LSB, len,
                     SDL_MIX_MAXVOLUME);
}

int init() {
  dst_filename = "res.pcm";

  dst_file = fopen(dst_filename, "wb");
  if (!dst_file) {
    fprintf(stderr, "Could not open destination file %s\n", dst_filename);
    exit(1);
  }

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

  /* compute the number of converted samples: buffering is avoided
   * ensuring that the output buffer will contain at least all the
   * converted input samples */
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

int main() {
  init();

  SDL_Init(SDL_INIT_AUDIO);

  // 16Mb should be enough; the test lasts 5 seconds
  buffer = (Uint8 *)malloc(16777215);

  SDL_AudioSpec want_in, want_out, have_in, have_out;

  SDL_zero(want_in);
  want_in.freq = 48000;
  want_in.format = AUDIO_S16LSB;
  want_in.channels = 1;
  want_in.samples = 480;
  want_in.callback = cb_in;

  input_dev = SDL_OpenAudioDevice(NULL, 1, &want_in, &have_in, 0);

  printf("%d %d %d %d\n", have_in.freq, have_in.format, have_in.channels,
         have_in.samples);
  if (input_dev == 0) {
    SDL_Log("Failed to open input: %s", SDL_GetError());
    return 1;
  }

  SDL_zero(want_out);
  want_out.freq = 48000;
  want_out.format = AUDIO_S16LSB;
  want_out.channels = 1;
  want_out.samples = 480;
  want_out.callback = cb_out;

  output_dev = SDL_OpenAudioDevice(NULL, 0, &want_out, &have_out, 0);

  printf("%d %d %d %d\n", have_out.freq, have_out.format, have_out.channels,
         have_out.samples);
  if (output_dev == 0) {
    SDL_Log("Failed to open input: %s", SDL_GetError());
    return 1;
  }

  SDL_PauseAudioDevice(input_dev, 0);
  SDL_PauseAudioDevice(output_dev, 0);

  while (1) {
  }

  SDL_CloseAudioDevice(output_dev);
  SDL_CloseAudioDevice(input_dev);
  free(buffer);

  fclose(outfile);
}
