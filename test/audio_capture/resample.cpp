#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int ret;
  AVFrame *frame = NULL;
  AVFrame *resampled_frame = NULL;
  AVCodecContext *codec_ctx = NULL;
  SwrContext *swr_ctx = NULL;

  // Initialize FFmpeg
  av_log_set_level(AV_LOG_INFO);
  av_register_all();

  // Allocate input frame
  frame = av_frame_alloc();
  if (!frame) {
    av_log(NULL, AV_LOG_ERROR, "Failed to allocate input frame\n");
    return -1;
  }

  // Allocate output frame for resampled data
  resampled_frame = av_frame_alloc();
  if (!resampled_frame) {
    av_log(NULL, AV_LOG_ERROR, "Failed to allocate output frame\n");
    return -1;
  }

  // Set input frame properties
  frame->format = AV_SAMPLE_FMT_FLTP;  // Input sample format (float planar)
  frame->channel_layout = AV_CH_LAYOUT_STEREO;  // Input channel layout (stereo)
  frame->sample_rate = 44100;                   // Input sample rate (44100 Hz)
  frame->nb_samples = 1024;                     // Number of input samples

  // Set output frame properties
  resampled_frame->format =
      AV_SAMPLE_FMT_S16;  // Output sample format (signed 16-bit)
  resampled_frame->channel_layout =
      AV_CH_LAYOUT_STEREO;               // Output channel layout (stereo)
  resampled_frame->sample_rate = 48000;  // Output sample rate (48000 Hz)
  resampled_frame->nb_samples = av_rescale_rnd(
      frame->nb_samples, resampled_frame->sample_rate, frame->sample_rate,
      AV_ROUND_UP);  // Number of output samples

  // Initialize resampler context
  swr_ctx = swr_alloc_set_opts(
      NULL, av_get_default_channel_layout(resampled_frame->channel_layout),
      av_get_default_sample_fmt(resampled_frame->format),
      resampled_frame->sample_rate,
      av_get_default_channel_layout(frame->channel_layout),
      av_get_default_sample_fmt(frame->format), frame->sample_rate, 0, NULL);
  if (!swr_ctx) {
    av_log(NULL, AV_LOG_ERROR, "Failed to allocate resampler context\n");
    return -1;
  }

  // Initialize and configure the resampler
  if ((ret = swr_init(swr_ctx)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Failed to initialize resampler context: %s\n",
           av_err2str(ret));
    return -1;
  }

  // Allocate buffer for output samples
  ret = av_samples_alloc(resampled_frame->data, resampled_frame->linesize,
                         resampled_frame->channels, resampled_frame->nb_samples,
                         resampled_frame->format, 0);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Failed to allocate output samples buffer: %s\n",
           av_err2str(ret));
    return -1;
  }

  // Resample the input data
  ret = swr_convert(swr_ctx, resampled_frame->data, resampled_frame->nb_samples,
                    (const uint8_t **)frame->data, frame->nb_samples);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Failed to resample input data: %s\n",
           av_err2str(ret));
    return -1;
  }

  // Cleanup and free resources
  swr_free(&swr_ctx);
  av_frame_free(&frame);
  av_frame_free(&resampled_frame);

  return 0;
}
