#define _GNU_SOURCE
#include "video_sdl_player.h"
#include "image_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

double get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

VideoPlayer* video_player_init(const char* video_file, int window_width, int window_height) {
    if (!video_file) {
        fprintf(stderr, "Error: No video file specified\n");
        return NULL;
    }

    VideoPlayer* player = calloc(1, sizeof(VideoPlayer));
    if (!player) {
        fprintf(stderr, "Error: Cannot allocate memory for video player\n");
        return NULL;
    }

    player->video_processor = video_processor_init(video_file);
    if (!player->video_processor) {
        fprintf(stderr, "Error: Failed to initialize video processor\n");
        free(player);
        return NULL;
    }

    player->display = sdl_display_init(window_width, window_height);
    if (!player->display) {
        fprintf(stderr, "Error: Failed to initialize SDL display\n");
        video_processor_cleanup(player->video_processor);
        free(player);
        return NULL;
    }

    player->ascii_config = create_default_config();
    player->state = PLAYER_STOPPED;
    player->playback_speed = 1.0;
    player->current_frame = 0;
    player->total_frames = video_processor_get_total_frames(player->video_processor);
    player->original_fps = video_processor_get_fps(player->video_processor);
    player->target_fps = player->original_fps;
    player->show_controls = 1;
    player->show_stats = 1;
    // Override ASCII grid to 400×400 characters
    player->ascii_cols = 200;
    player->ascii_rows = 200;
    // Resize window to fit exactly 400×400 characters
    {
        SDL_SetWindowSize(player->display->window, 1100, 1100);
        int new_win_w = player->display->char_width * player->ascii_cols;
        int new_win_h = player->display->char_height * player->ascii_rows;
        player->display->window_width = new_win_w;
        player->display->window_height = new_win_h;
        player->display->ascii_width = new_win_w;
        player->display->ascii_height = new_win_h;
        player->display->video_width = new_win_w / 2;
        player->display->video_height = new_win_h;
    }
    player->frame_delay_ms = 1000.0 / player->target_fps;
    player->last_frame_time = get_current_time_ms();
    
    printf("Video Player Initialized:\n");
    printf("  Video: %s\n", video_file);
    printf("  Resolution: %dx%d\n", 
           video_processor_get_width(player->video_processor),
           video_processor_get_height(player->video_processor));
    printf("  FPS: %.2f\n", player->original_fps);
    printf("  Total frames: %ld\n", player->total_frames);
    printf("  ASCII dimensions: %dx%d characters\n", player->ascii_cols, player->ascii_rows);
    
    return player;
}

void video_player_cleanup(VideoPlayer* player) {
    if (!player) return;
    if (player->video_processor) video_processor_cleanup(player->video_processor);
    if (player->display) sdl_display_cleanup(player->display);
    free(player);
}

void video_player_play(VideoPlayer* player) {
    if (!player) return;
    if (player->state == PLAYER_STOPPED) {
        video_processor_reset(player->video_processor);
        player->current_frame = 0;
    }
    player->state = PLAYER_PLAYING;
    player->last_frame_time = get_current_time_ms();
    printf("Playing video...\n");
}

void video_player_pause(VideoPlayer* player) {
    if (!player) return;
    player->state = PLAYER_PAUSED;
    printf("Video paused\n");
}

void video_player_stop(VideoPlayer* player) {
    if (!player) return;
    player->state = PLAYER_STOPPED;
    video_processor_reset(player->video_processor);
    player->current_frame = 0;
    printf("Video stopped\n");
}

void video_player_set_speed(VideoPlayer* player, double speed) {
    if (!player || speed <= 0.0) return;
    player->playback_speed = speed;
    player->target_fps = player->original_fps * speed;
    player->frame_delay_ms = 1000.0 / player->target_fps;
    printf("Speed: %.2fx (%.2f FPS)\n", speed, player->target_fps);
}

int video_player_handle_events(VideoPlayer* player) {
    if (!player) return 0;
    
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                return 0;  // Quit
                
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        return 0;  // Quit
                        
                    case SDLK_SPACE:
                        if (player->state == PLAYER_PLAYING) {
                            video_player_pause(player);
                        } else {
                            video_player_play(player);
                        }
                        break;
                        
                    case SDLK_s:
                        video_player_stop(player);
                        break;
                        
                    case SDLK_RIGHT:
                        // Seek forward 10 frames
                        if (player->current_frame + 10 < player->total_frames) {
                            video_player_seek_frame(player, player->current_frame + 10);
                        }
                        break;
                        
                    case SDLK_LEFT:
                        // Seek backward 10 frames
                        if (player->current_frame >= 10) {
                            video_player_seek_frame(player, player->current_frame - 10);
                        }
                        break;
                        
                    case SDLK_UP:
                        // Increase speed
                        video_player_set_speed(player, player->playback_speed * 1.25);
                        break;
                        
                    case SDLK_DOWN:
                        // Decrease speed
                        video_player_set_speed(player, player->playback_speed / 1.25);
                        break;
                        
                    case SDLK_1:
                    case SDLK_2:
                    case SDLK_3:
                    case SDLK_4:
                    case SDLK_5:
                    case SDLK_6:
                        // Character set selection
                        player->ascii_config.char_set_index = e.key.keysym.sym - SDLK_1;
                        if (player->ascii_config.char_set_index >= NUM_ASCII_SETS) {
                            player->ascii_config.char_set_index = 0;
                        }
                        printf("Switched to character set %d: %s\n", 
                               player->ascii_config.char_set_index + 1,
                               ASCII_SETS[player->ascii_config.char_set_index].name);
                        break;
                        
                    case SDLK_i:
                        // Invert brightness
                        player->ascii_config.invert_brightness = !player->ascii_config.invert_brightness;
                        printf("Brightness inversion: %s\n", 
                               player->ascii_config.invert_brightness ? "ON" : "OFF");
                        break;
                        
                    case SDLK_r:
                        // Reset settings
                        player->ascii_config = create_default_config();
                        video_player_set_speed(player, 1.0);
                        printf("Reset to default settings\n");
                        break;
                        
                    case SDLK_h:
                        // Toggle help/controls display
                        player->show_controls = !player->show_controls;
                        if (player->show_controls) {
                            video_player_print_controls();
                        }
                        break;
                        
                    case SDLK_t:
                        // Toggle stats display
                        player->show_stats = !player->show_stats;
                        break;
                }
                break;
        }
    }
    
    return 1;
}

void video_player_seek_frame(VideoPlayer* player, int64_t frame) {
    if (!player || frame < 0 || frame >= player->total_frames) return;

    video_processor_reset(player->video_processor);
    player->current_frame = 0;

    // Skip frames to reach target
    for (int64_t i = 0; i < frame; i++) {
        Image* temp_frame = video_processor_get_next_frame(player->video_processor);
        if (temp_frame) {
            free_image(temp_frame);
            player->current_frame++;
        } else {
            break;
        }
    }

    printf("Seeked to frame %ld\n", player->current_frame);
}

void video_player_update_display(VideoPlayer* player, const Image* frame, const char* ascii_art) {
    if (!player || !player->display) return;

    // Create performance stats
    SDLPerformanceStats stats = {0};
    if (player->show_stats) {
        stats.fps = player->target_fps;
        stats.frame_count = (int)player->current_frame;
        stats.avg_process_time = 0.0; // Could be calculated
        stats.last_frame_time = get_current_time_ms() - player->last_frame_time;
    }

    // Display the frame
    sdl_display_frame_split(player->display, frame, ascii_art, player->show_stats ? &stats : NULL);
}

// Main video player run loop
int video_player_run(VideoPlayer* player) {
    if (!player) return -1;

    printf("Starting video player...\n");
    video_player_print_controls();

    // Start playing automatically
    video_player_play(player);

    while (1) {
        double current_time = get_current_time_ms();

        // Handle events
        if (!video_player_handle_events(player)) {
            break; // Quit requested
        }

        // Process frame if playing
        if (player->state == PLAYER_PLAYING) {
            double time_since_last_frame = current_time - player->last_frame_time;

            if (time_since_last_frame >= player->frame_delay_ms) {
                // Get next frame
                Image* frame = video_processor_get_next_frame(player->video_processor);
                if (frame) {
                    player->current_frame++;

                    // Create ASCII version (no need for display image anymore)
                    Image* ascii_img = resize_image_aspect_ratio(frame,
                                                               player->ascii_cols,
                                                               player->ascii_rows);

                    char* ascii_art = NULL;
                    if (ascii_img) {
                        ascii_art = image_to_ascii(ascii_img, &player->ascii_config);
                    }

                    // Update display (pass NULL for video image)
                    video_player_update_display(player, NULL, ascii_art);

                    // Cleanup
                    if (ascii_art) free(ascii_art);
                    if (ascii_img) free_image(ascii_img);
                    free_image(frame);

                    player->last_frame_time = current_time;
                } else {
                    // End of video - loop back to beginning
                    printf("End of video reached - looping...\n");
                    video_processor_reset(player->video_processor);
                    player->current_frame = 0;
                    player->last_frame_time = current_time;
                }
            }
        }

        // Small delay to prevent excessive CPU usage
        SDL_Delay(1);
    }

    printf("Video player stopped\n");
    return 0;
}

void video_player_print_controls(void) {
    printf("\n=== Controls ===\n");
    printf("SPACE: Play/Pause | S: Stop | LEFT/RIGHT: Seek\n");
    printf("UP/DOWN: Speed | 1-6: Character sets | I: Invert\n");
    printf("R: Reset | Q/ESC: Quit | Video loops automatically\n");
    printf("================\n\n");
}
