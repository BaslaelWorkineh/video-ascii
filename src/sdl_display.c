#include "sdl_display.h"
#include "image_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SDLDisplay* sdl_display_init(int width, int height) {
    SDLDisplay* display = malloc(sizeof(SDLDisplay));
    if (!display) return NULL;
    
    memset(display, 0, sizeof(SDLDisplay));
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        free(display);
        return NULL;
    }
    
    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        SDL_Quit();
        free(display);
        return NULL;
    }
    
    display->font_size = DEFAULT_FONT_SIZE;

    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/System/Library/Fonts/Monaco.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        NULL
    };

    for (int i = 0; font_paths[i] != NULL; i++) {
        display->font = TTF_OpenFont(font_paths[i], display->font_size);
        if (display->font) break;
    }

    if (!display->font) {
        fprintf(stderr, "Warning: Could not load any font, using default\n");
        display->char_width = 7;
        display->char_height = 14;
    } else {
        int w, h;
        TTF_SizeText(display->font, "M", &w, &h);
        display->char_width = w;
        display->char_height = h;
    }

    int ascii_cols = width / display->char_width;
    int ascii_rows = height / display->char_height;

    display->window_width = ascii_cols * display->char_width;
    display->window_height = ascii_rows * display->char_height;
    display->video_width = display->window_width;
    display->video_height = display->window_height;
    display->ascii_width = display->window_width;
    display->ascii_height = display->window_height;
    display->font_size = DEFAULT_FONT_SIZE;
    
    display->window = SDL_CreateWindow("ASCII Video Player",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      display->window_width,
                                      display->window_height,
                                      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    if (!display->window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        free(display);
        return NULL;
    }
    
    display->renderer = SDL_CreateRenderer(display->window, -1, 
                                          SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!display->renderer) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(display->window);
        TTF_Quit();
        SDL_Quit();
        free(display);
        return NULL;
    }
    
    if (!display->font) {
        fprintf(stderr, "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(display->renderer);
        SDL_DestroyWindow(display->window);
        TTF_Quit();
        SDL_Quit();
        free(display);
        return NULL;
    }
    
    printf("SDL Display initialized: %dx%d window, font size: %d, char: %dx%d\n",
           display->window_width, display->window_height, 
           display->font_size, display->char_width, display->char_height);
    
    return display;
}

SDL_Texture* create_texture_from_image(SDLDisplay* display, const Image* img) {
    if (!display || !img) return NULL;
    
    SDL_Texture* texture = SDL_CreateTexture(display->renderer,
                                           SDL_PIXELFORMAT_RGB24,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           img->width, img->height);
    
    if (!texture) return NULL;
    
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(texture, NULL, &pixels, &pitch) == 0) {
        if (img->channels == 3) {
            memcpy(pixels, img->data, img->width * img->height * 3);
        } else if (img->channels == 1) {
            uint8_t* rgb_pixels = (uint8_t*)pixels;
            for (int i = 0; i < img->width * img->height; i++) {
                uint8_t gray = img->data[i];
                rgb_pixels[i * 3] = gray;    
                rgb_pixels[i * 3 + 1] = gray; 
                rgb_pixels[i * 3 + 2] = gray; 
            }
        }
        SDL_UnlockTexture(texture);
    }
    
    return texture;
}

SDL_Texture* create_texture_from_ascii(SDLDisplay* display, const char* ascii_art) {
    if (!display || !ascii_art) return NULL;
    
    SDL_Texture* texture = SDL_CreateTexture(display->renderer,
                                           SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET,
                                           display->ascii_width, display->ascii_height);
    
    if (!texture) return NULL;
    
    SDL_SetRenderTarget(display->renderer, texture);
    
    SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
    SDL_RenderClear(display->renderer);
    
    SDL_Color white = {255, 255, 255, 255};

    int x = 0, y = 0; 
    const char* ptr = ascii_art;
    char line[1024];
    int line_pos = 0;
    
    while (*ptr) {
        if (*ptr == '\n' || line_pos >= (int)sizeof(line) - 1) {
            line[line_pos] = '\0';
            if (line_pos > 0) {
                SDL_Surface* text_surface = TTF_RenderText_Solid(display->font, line, white);
                if (text_surface) {
                    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(display->renderer, text_surface);
                    if (text_texture) {
                        SDL_Rect dst_rect = {x, y, text_surface->w, text_surface->h};
                        SDL_RenderCopy(display->renderer, text_texture, NULL, &dst_rect);
                        SDL_DestroyTexture(text_texture);
                    }
                    SDL_FreeSurface(text_surface);
                }
            }
            y += display->char_height;
            line_pos = 0;

            if (y >= display->ascii_height) break;
        } else {
            line[line_pos++] = *ptr;
        }
        ptr++;
    }

    SDL_SetRenderTarget(display->renderer, NULL);
    
    return texture;
}


int sdl_display_frame_split(SDLDisplay* display, const Image* img, const char* ascii_art, SDLPerformanceStats* stats) {
    (void)img;
    if (!display) return -1;

    SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
    SDL_RenderClear(display->renderer);

    if (ascii_art) {
        SDL_Texture* ascii_texture = create_texture_from_ascii(display, ascii_art);
        if (ascii_texture) {
            SDL_Rect ascii_rect = {0, 0, display->ascii_width, display->ascii_height};
            SDL_RenderCopy(display->renderer, ascii_texture, NULL, &ascii_rect);
            SDL_DestroyTexture(ascii_texture);
        }
    }
    
    if (stats) {
        char stats_text[256];
        snprintf(stats_text, sizeof(stats_text), "FPS: %.1f | Frames: %d | Process: %.1fms", 
                stats->fps, stats->frame_count, stats->avg_process_time);
        
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* text_surface = TTF_RenderText_Solid(display->font, stats_text, white);
        if (text_surface) {
            SDL_Texture* text_texture = SDL_CreateTextureFromSurface(display->renderer, text_surface);
            if (text_texture) {
                SDL_Rect text_rect = {10, display->window_height - 30, text_surface->w, text_surface->h};
                SDL_RenderCopy(display->renderer, text_texture, NULL, &text_rect);
                SDL_DestroyTexture(text_texture);
            }
            SDL_FreeSurface(text_surface);
        }
    }
    
    SDL_RenderPresent(display->renderer);
    
    return 0;
}

int sdl_handle_events(void) {
    SDL_Event e;
    
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                return 0;
                
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        return 0;
                        
                    case SDLK_1:
                    case SDLK_2:
                    case SDLK_3:
                    case SDLK_4:
                    case SDLK_5:
                    case SDLK_6:
                        return e.key.keysym.sym - SDLK_0;
                        
                    case SDLK_i:
                        return 10; 
                        
                    case SDLK_r:
                        return 11; 
                }
                break;
        }
    }
    
    return 1; 
}

void sdl_display_cleanup(SDLDisplay* display) {
    if (display) {
        if (display->font) TTF_CloseFont(display->font);
        if (display->renderer) SDL_DestroyRenderer(display->renderer);
        if (display->window) SDL_DestroyWindow(display->window);
        free(display);
    }
    
    TTF_Quit();
    SDL_Quit();
}
