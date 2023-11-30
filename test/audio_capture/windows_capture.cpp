#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libswresample/swresample.h>
}

#include <windows.h>

#include <memory>
#include <string>
#include <vector>

#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")

#pragma comment(lib, "Winmm.lib")

using std::shared_ptr;
using std::string;
using std::vector;

void capture_audio() {
  // windows api ��ȡ��Ƶ�豸�б�ffmpeg ����û���ṩ��ȡ����Ƶ�豸��api��
  int nDeviceNum = waveInGetNumDevs();
  vector<string> vecDeviceName;
  for (int i = 0; i < nDeviceNum; ++i) {
    WAVEINCAPS wic;
    waveInGetDevCaps(i, &wic, sizeof(wic));

    // ת��utf-8
    int nSize = WideCharToMultiByte(CP_UTF8, 0, wic.szPname,
                                    wcslen(wic.szPname), NULL, 0, NULL, NULL);
    shared_ptr<char> spDeviceName(new char[nSize + 1]);
    memset(spDeviceName.get(), 0, nSize + 1);
    WideCharToMultiByte(CP_UTF8, 0, wic.szPname, wcslen(wic.szPname),
                        spDeviceName.get(), nSize, NULL, NULL);
    vecDeviceName.push_back(spDeviceName.get());
    av_log(NULL, AV_LOG_DEBUG, "audio input device : %s \n",
           spDeviceName.get());
  }
  if (vecDeviceName.size() <= 0) {
    av_log(NULL, AV_LOG_ERROR, "not find audio input device.\n");
    return;
  }
  string sDeviceName = "audio=" + vecDeviceName[0];  // ʹ�õ�һ����Ƶ�豸

  // ffmpeg
  avdevice_register_all();  // ע��������������豸
  AVInputFormat* ifmt =
      (AVInputFormat*)av_find_input_format("dshow");  // ���òɼ���ʽ dshow
  if (ifmt == NULL) {
    av_log(NULL, AV_LOG_ERROR, "av_find_input_format for dshow fail.\n");
    return;
  }

  AVFormatContext* fmt_ctx = NULL;  // format ������
  int ret = avformat_open_input(&fmt_ctx, sDeviceName.c_str(), ifmt,
                                NULL);  // ����Ƶ�豸
  if (ret != 0) {
    av_log(NULL, AV_LOG_ERROR, "avformat_open_input fail. return %d.\n", ret);
    return;
  }

  AVPacket pkt;

  int64_t src_rate = 44100;
  int64_t dst_rate = 48000;
  SwrContext* swr_ctx = swr_alloc();

  uint8_t** dst_data = NULL;
  int dst_linesize = 0;

  // �����������
  av_opt_set_int(swr_ctx, "in_channel_layout", AV_CH_LAYOUT_MONO, 0);
  av_opt_set_int(swr_ctx, "in_sample_rate", src_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
  // �����������
  av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
  av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
  // ��ʼ��SwrContext
  swr_init(swr_ctx);

  FILE* fp = fopen("dst.pcm", "wb");
  int count = 0;
  while (count++ < 10) {
    ret = av_read_frame(fmt_ctx, &pkt);
    if (ret != 0) {
      av_log(NULL, AV_LOG_ERROR, "av_read_frame fail, return %d .\n", ret);
      break;
    }

    int out_samples_per_channel =
        (int)av_rescale_rnd(1024, dst_rate, src_rate, AV_ROUND_UP);
    int out_buffer_size = av_samples_get_buffer_size(
        NULL, 1, out_samples_per_channel, AV_SAMPLE_FMT_S16, 0);
    // uint8_t* out_buffer = (uint8_t*)av_malloc(out_buffer_size);
    ret = av_samples_alloc_array_and_samples(
        &dst_data, &dst_linesize, 2, out_buffer_size, AV_SAMPLE_FMT_S16, 0);

    // �����ز���
    swr_convert(swr_ctx, dst_data, out_samples_per_channel,
                (const uint8_t**)&pkt.data, 1024);

    fwrite(dst_data[1], 1, out_buffer_size, fp);
    av_packet_unref(&pkt);  // �����ͷ�pkt������ڴ棬������ڴ�й¶
  }
  fflush(fp);  // ˢ���ļ�io���������
  fclose(fp);

  avformat_close_input(&fmt_ctx);
}

int main(int argc, char** argv) {
  av_log_set_level(AV_LOG_DEBUG);  // ����ffmpeg��־��ȼ�
  capture_audio();

  Sleep(1);
}