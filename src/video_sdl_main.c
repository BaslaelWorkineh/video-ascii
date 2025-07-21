#include "video_sdl_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char* program_name) {
    printf("SDL2 Video to ASCII Player\n");
    printf("==========================\n\n");
    printf("Usage: %s <video_file> [options]\n\n", program_name);
    printf("Options:\n");
    printf("  -w <width>   Window width (default: 500)\n");
    printf("  -h <height>  Window height (default: 500)\n");
    printf("  --help       Show this help message\n\n");
    printf("Controls:\n");
    printf("  SPACE:       Play/Pause\n");
    printf("  S:           Stop\n");
    printf("  LEFT/RIGHT:  Seek backward/forward\n");
    printf("  UP/DOWN:     Speed control\n");
    printf("  1-6:         ASCII character sets\n");
    printf("  I:           Invert brightness\n");
    printf("  R:           Reset settings\n");
    printf("  Q/ESC:       Quit\n\n");
    printf("Character sets:\n");
    print_available_charsets();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    const char* video_file = argv[1];
    int window_width = 500;
    int window_height = 500;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            window_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            window_height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (window_width <= 0 || window_height <= 0) {
        fprintf(stderr, "Error: Invalid window dimensions\n");
        return 1;
    }

    printf("SDL2 Video to ASCII Player\n");
    printf("Video: %s | Size: %dx%d\n", video_file, window_width, window_height);

    VideoPlayer* player = video_player_init(video_file, window_width, window_height);
    if (!player) {
        fprintf(stderr, "Error: Failed to initialize video player\n");
        return 1;
    }

    int result = video_player_run(player);
    video_player_cleanup(player);

    return result;
}
