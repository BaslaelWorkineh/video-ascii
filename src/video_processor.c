#include "video_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VideoProcessor* video_processor_init(const char* filename) {
    if (!filename) return NULL;

    VideoProcessor* vp = calloc(1, sizeof(VideoProcessor));
    if (!vp) return NULL;

    if (avformat_open_input(&vp->format_ctx, filename, NULL, NULL) < 0) {
        free(vp);
        return NULL;
    }

    if (avformat_find_stream_info(vp->format_ctx, NULL) < 0) {
        avformat_close_input(&vp->format_ctx);
        free(vp);
        return NULL;
    }

    vp->video_stream_index = -1;
    for (unsigned int i = 0; i < vp->format_ctx->nb_streams; i++) {
        if (vp->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            vp->video_stream_index = i;
            break;
        }
    }

    if (vp->video_stream_index == -1) {
        avformat_close_input(&vp->format_ctx);
        free(vp);
        return NULL;
    }

    AVCodecParameters* codecpar = vp->format_ctx->streams[vp->video_stream_index]->codecpar;

    vp->codec = avcodec_find_decoder(codecpar->codec_id);
    if (!vp->codec) {
        avformat_close_input(&vp->format_ctx);
        free(vp);
        return NULL;
    }
    
    vp->codec_ctx = avcodec_alloc_context3(vp->codec);
    if (!vp->codec_ctx) {
        fprintf(stderr, "Error: Cannot allocate codec context\n");
        avformat_close_input(&vp->format_ctx);
        free(vp);
        return NULL;
    }
    
    if (avcodec_parameters_to_context(vp->codec_ctx, codecpar) < 0) {
        fprintf(stderr, "Error: Cannot copy codec parameters\n");
        avcodec_free_context(&vp->codec_ctx);
        avformat_close_input(&vp->format_ctx);
        free(vp);
        return NULL;
    }
    
    if (avcodec_open2(vp->codec_ctx, vp->codec, NULL) < 0) {
        fprintf(stderr, "Error: Cannot open codec\n");
        avcodec_free_context(&vp->codec_ctx);
        avformat_close_input(&vp->format_ctx);
        free(vp);
        return NULL;
    }
    
    vp->frame = av_frame_alloc();
    vp->rgb_frame = av_frame_alloc();
    vp->packet = av_packet_alloc();
    
    if (!vp->frame || !vp->rgb_frame || !vp->packet) {
        fprintf(stderr, "Error: Cannot allocate frames/packet\n");
        video_processor_cleanup(vp);
        return NULL;
    }
    
    vp->width = vp->codec_ctx->width;
    vp->height = vp->codec_ctx->height;
    
    AVRational time_base = vp->format_ctx->streams[vp->video_stream_index]->time_base;
    AVRational frame_rate = vp->format_ctx->streams[vp->video_stream_index]->r_frame_rate;
    
    if (frame_rate.num > 0 && frame_rate.den > 0) {
        vp->fps = (double)frame_rate.num / frame_rate.den;
    } else {
        vp->fps = 1.0 / av_q2d(time_base);
    }
    
    int64_t duration = vp->format_ctx->duration;
    if (duration != AV_NOPTS_VALUE) {
        vp->total_frames = (int64_t)(duration * vp->fps / AV_TIME_BASE);
    } else {
        vp->total_frames = -1; // Unknown
    }
    
    vp->current_frame = 0;
    
    vp->sws_ctx = sws_getContext(
        vp->width, vp->height, vp->codec_ctx->pix_fmt,
        vp->width, vp->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL
    );
    
    if (!vp->sws_ctx) {
        fprintf(stderr, "Error: Cannot initialize scaling context\n");
        video_processor_cleanup(vp);
        return NULL;
    }
    
    vp->rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, vp->width, vp->height, 1);
    vp->rgb_buffer = av_malloc(vp->rgb_buffer_size);
    
    if (!vp->rgb_buffer) {
        fprintf(stderr, "Error: Cannot allocate RGB buffer\n");
        video_processor_cleanup(vp);
        return NULL;
    }
    
    av_image_fill_arrays(vp->rgb_frame->data, vp->rgb_frame->linesize,
                        vp->rgb_buffer, AV_PIX_FMT_RGB24, vp->width, vp->height, 1);
    
    return vp;
}

void video_processor_cleanup(VideoProcessor* vp) {
    if (!vp) return;

    if (vp->rgb_buffer) {
        av_free(vp->rgb_buffer);
    }

    if (vp->sws_ctx) {
        sws_freeContext(vp->sws_ctx);
    }

    if (vp->frame) {
        av_frame_free(&vp->frame);
    }

    if (vp->rgb_frame) {
        av_frame_free(&vp->rgb_frame);
    }

    if (vp->packet) {
        av_packet_free(&vp->packet);
    }

    if (vp->codec_ctx) {
        avcodec_free_context(&vp->codec_ctx);
    }

    if (vp->format_ctx) {
        avformat_close_input(&vp->format_ctx);
    }

    free(vp);
}

Image* video_processor_get_next_frame(VideoProcessor* vp) {
    if (!vp || !video_processor_is_valid(vp)) {
        return NULL;
    }

    int ret;

    while ((ret = av_read_frame(vp->format_ctx, vp->packet)) >= 0) {
        if (vp->packet->stream_index == vp->video_stream_index) {
            ret = avcodec_send_packet(vp->codec_ctx, vp->packet);
            if (ret < 0) {
                av_packet_unref(vp->packet);
                continue;
            }

            ret = avcodec_receive_frame(vp->codec_ctx, vp->frame);
            if (ret == 0) {
                av_packet_unref(vp->packet);

                sws_scale(vp->sws_ctx,
                         (const uint8_t* const*)vp->frame->data, vp->frame->linesize,
                         0, vp->height,
                         vp->rgb_frame->data, vp->rgb_frame->linesize);

                Image* img = create_image(vp->width, vp->height, 3);
                if (img) {
                    memcpy(img->data, vp->rgb_buffer, vp->rgb_buffer_size);
                    vp->current_frame++;
                }

                return img;
            }
        }
        av_packet_unref(vp->packet);
    }

    return NULL;
}

void video_processor_reset(VideoProcessor* vp) {
    if (!vp || !video_processor_is_valid(vp)) {
        return;
    }

    av_seek_frame(vp->format_ctx, vp->video_stream_index, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(vp->codec_ctx);
    vp->current_frame = 0;
}

double video_processor_get_duration(VideoProcessor* vp) {
    if (!vp || !video_processor_is_valid(vp)) {
        return -1.0;
    }

    if (vp->format_ctx->duration != AV_NOPTS_VALUE) {
        return (double)vp->format_ctx->duration / AV_TIME_BASE;
    }

    return -1.0;
}

double video_processor_get_fps(VideoProcessor* vp) {
    if (!vp) return -1.0;
    return vp->fps;
}

int video_processor_get_width(VideoProcessor* vp) {
    if (!vp) return -1;
    return vp->width;
}

int video_processor_get_height(VideoProcessor* vp) {
    if (!vp) return -1;
    return vp->height;
}

int64_t video_processor_get_total_frames(VideoProcessor* vp) {
    if (!vp) return -1;
    return vp->total_frames;
}

int64_t video_processor_get_current_frame(VideoProcessor* vp) {
    if (!vp) return -1;
    return vp->current_frame;
}

int video_processor_is_valid(VideoProcessor* vp) {
    return vp && vp->format_ctx && vp->codec_ctx && vp->frame && vp->rgb_frame && vp->packet;
}

void video_processor_print_info(VideoProcessor* vp) {
    if (!vp || !video_processor_is_valid(vp)) {
        printf("Invalid video processor\n");
        return;
    }

    printf("Video Information:\n");
    printf("  Resolution: %dx%d\n", vp->width, vp->height);
    printf("  FPS: %.2f\n", vp->fps);
    printf("  Duration: %.2f seconds\n", video_processor_get_duration(vp));
    if (vp->total_frames > 0) {
        printf("  Total frames: %ld (estimated)\n", vp->total_frames);
    } else {
        printf("  Total frames: Unknown\n");
    }
    printf("  Current frame: %ld\n", vp->current_frame);
}
