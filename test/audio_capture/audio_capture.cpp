#include <stdio.h>

#ifdef _WIN32
// Windows
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};
#else
// Linux...
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
};
#endif
#endif

int main(int argc, char **argv) {
  int ret = 0;
  char errors[1024] = {0};
  // context
  AVFormatContext *fmt_ctx = NULL;  // ffmpeg下的“文件描述符”

  // paket
  int count = 0;
  AVPacket pkt;

  // create file
  char *out = "audio_old.pcm";
  FILE *outfile = fopen(out, "wb+");

  char *devicename = "default";
  // register audio device
  avdevice_register_all();

  // get format
  AVInputFormat *iformat = (AVInputFormat *)av_find_input_format("sndio");

  // open audio
  if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, NULL)) < 0) {
    av_strerror(ret, errors, 1024);
    printf("Failed to open audio device, [%d]%s\n", ret, errors);
    return -1;
  }

  // 查找音频流信息
  if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
    printf("111\n");
    return -1;
  }

  // 寻找第一个音频流索引
  int audioStreamIndex =
      av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  if (audioStreamIndex < 0) {
    printf("222\n");
    return -1;
  }

  av_init_packet(&pkt);
  // read data form audio
  while (ret = (av_read_frame(fmt_ctx, &pkt)) == 0 && count++ < 10000) {
    av_log(NULL, AV_LOG_INFO, "pkt size is %d(%p), count=%d\n", pkt.size,
           pkt.data, count);
    fwrite(pkt.data, 1, pkt.size, outfile);
    fflush(outfile);
    av_packet_unref(&pkt);  // release pkt
  }

  fclose(outfile);
  avformat_close_input(&fmt_ctx);  // releas ctx

  return 0;
}