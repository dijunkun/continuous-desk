#pragma once
extern "C" {
#include <libavcodec\adts_parser.h>
#include <libavcodec\avcodec.h>
#include <libavdevice\avdevice.h>
#include <libavfilter\avfilter.h>
#include <libavfilter\buffersink.h>
#include <libavfilter\buffersrc.h>
#include <libavformat\avformat.h>
#include <libavutil\avassert.h>
#include <libavutil\channel_layout.h>
#include <libavutil\error.h>
#include <libavutil\imgutils.h>
#include <libavutil\log.h>
#include <libavutil\mathematics.h>
#include <libavutil\opt.h>
#include <libavutil\samplefmt.h>
#include <libavutil\time.h>
#include <libavutil\timestamp.h>
#include <libswresample\swresample.h>
#include <libswscale\swscale.h>
}