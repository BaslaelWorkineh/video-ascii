#ifndef VIDEO_SDL_PLAYER_H
#define VIDEO_SDL_PLAYER_H

#include "video_processor.h"
#include "sdl_display.h"
#include "ascii_converter.h"

typedef enum {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED
} PlayerState;

typedef struct {
    VideoProcessor* video_processor;
    SDLDisplay* display;
    AsciiConfig ascii_config;
    PlayerState state;
    double playback_speed;
    int64_t current_frame;
    int64_t total_frames;
    double target_fps;
    double original_fps;
    int ascii_cols;
    int ascii_rows;
    int show_controls;
    int show_stats;
    double last_frame_time;
    double frame_delay_ms;
} VideoPlayer;

VideoPlayer* video_player_init(const char* video_file, int window_width, int window_height);
void video_player_cleanup(VideoPlayer* player);
int video_player_run(VideoPlayer* player);
void video_player_play(VideoPlayer* player);
void video_player_pause(VideoPlayer* player);
void video_player_stop(VideoPlayer* player);
void video_player_set_speed(VideoPlayer* player, double speed);
void video_player_seek_frame(VideoPlayer* player, int64_t frame);
int video_player_handle_events(VideoPlayer* player);
void video_player_update_display(VideoPlayer* player, const Image* frame, const char* ascii_art);
double get_current_time_ms(void);
void video_player_print_controls(void);

#endif
