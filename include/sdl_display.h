#ifndef SDL_DISPLAY_H
#define SDL_DISPLAY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "image_loader.h"

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* video_texture;
    SDL_Texture* ascii_texture;
    TTF_Font* font;
    int window_width;
    int window_height;
    int video_width;
    int video_height;
    int ascii_width;
    int ascii_height;
    int font_size;
    int char_width;
    int char_height;
} SDLDisplay;

typedef struct {
    double fps;
    int frame_count;
    double avg_process_time;
    double last_frame_time;
} SDLPerformanceStats;

SDLDisplay* sdl_display_init(int width, int height);
void sdl_display_cleanup(SDLDisplay* display);
int sdl_display_frame_split(SDLDisplay* display, const Image* img, const char* ascii_art, SDLPerformanceStats* stats);

#define DEFAULT_FONT_SIZE 8

#endif
