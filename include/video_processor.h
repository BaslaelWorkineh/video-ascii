#ifndef VIDEO_PROCESSOR_H
#define VIDEO_PROCESSOR_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "image_loader.h"

typedef struct {
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx;
    AVCodec* codec;
    AVFrame* frame;
    AVFrame* rgb_frame;
    AVPacket* packet;
    struct SwsContext* sws_ctx;
    int video_stream_index;
    int width;
    int height;
    double fps;
    int64_t total_frames;
    int64_t current_frame;
    uint8_t* rgb_buffer;
    int rgb_buffer_size;
} VideoProcessor;

VideoProcessor* video_processor_init(const char* filename);
void video_processor_cleanup(VideoProcessor* vp);
Image* video_processor_get_next_frame(VideoProcessor* vp);
void video_processor_reset(VideoProcessor* vp);
double video_processor_get_fps(VideoProcessor* vp);
int video_processor_get_width(VideoProcessor* vp);
int video_processor_get_height(VideoProcessor* vp);
int64_t video_processor_get_total_frames(VideoProcessor* vp);
int64_t video_processor_get_current_frame(VideoProcessor* vp);
int video_processor_is_valid(VideoProcessor* vp);
void video_processor_print_info(VideoProcessor* vp);

#endif
