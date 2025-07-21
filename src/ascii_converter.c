#include "ascii_converter.h"
#include "image_processing.h"
#include <stdlib.h>
#include <string.h>

const AsciiCharSet ASCII_SETS[] = {
    {"@%#*+=-:. ", 10, "Standard"},
    {"█▉▊▋▌▍▎▏ ", 9, "Block"},
    {"@&%$#+=*:~-,. ", 14, "Extended"},
    {"#*+=-:. ", 8, "Simple"},
    {"██▓▒░  ", 7, "Shaded"},
    {"@#S%?*+;:,. ", 12, "Classic"}
};

const int NUM_ASCII_SETS = sizeof(ASCII_SETS) / sizeof(ASCII_SETS[0]);

AsciiConfig create_default_config(void) {
    AsciiConfig config;
    config.char_set_index = 0;
    config.invert_brightness = 0;
    config.aspect_ratio_correction = 0.5;  
    return config;
}

char brightness_to_ascii(uint8_t brightness, const AsciiCharSet* char_set, int invert) {
    if (!char_set || char_set->length == 0) return ' ';
    
    int index;
    if (invert) {
        index = (brightness * (char_set->length - 1)) / 255;
    } else {
        index = ((255 - brightness) * (char_set->length - 1)) / 255;
    }
    
    if (index < 0) index = 0;
    if (index >= char_set->length) index = char_set->length - 1;
    
    return char_set->chars[index];
}

char* image_to_ascii(const Image* img, const AsciiConfig* config) {
    if (!img || !img->data || !config) return NULL;
    
    if (config->char_set_index < 0 || config->char_set_index >= NUM_ASCII_SETS) {
        fprintf(stderr, "Error: Invalid character set index\n");
        return NULL;
    }
    
    const AsciiCharSet* char_set = &ASCII_SETS[config->char_set_index];
    
    Image* gray_img = NULL;
    const Image* work_img = img;
    
    if (img->channels > 1) {
        gray_img = convert_to_grayscale(img);
        if (!gray_img) {
            fprintf(stderr, "Error: Failed to convert image to grayscale\n");
            return NULL;
        }
        work_img = gray_img;
    }
    
    int output_width = work_img->width;
    int output_height = (int)(work_img->height * config->aspect_ratio_correction);
    
    char* ascii_art = malloc((output_width + 1) * output_height + 1);
    if (!ascii_art) {
        fprintf(stderr, "Error: Cannot allocate memory for ASCII art\n");
        if (gray_img) free_image(gray_img);
        return NULL;
    }
    
    int ascii_index = 0;
    
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            int src_y = (int)((float)y / config->aspect_ratio_correction);
            if (src_y >= work_img->height) src_y = work_img->height - 1;
            
            uint8_t brightness = get_pixel_brightness(work_img, x, src_y);
            char ascii_char = brightness_to_ascii(brightness, char_set, config->invert_brightness);
            ascii_art[ascii_index++] = ascii_char;
        }
        ascii_art[ascii_index++] = '\n'; 
    }
    
    ascii_art[ascii_index] = '\0'; 
    
    if (gray_img) free_image(gray_img);
    
    return ascii_art;
}

void print_ascii_art(const char* ascii_art, int width, int height) {
    (void)width; (void)height;
    if (!ascii_art) return;

    printf("%s", ascii_art);
}

int save_ascii_to_file(const char* ascii_art, int width, int height, const char* filename) {
    (void)width; (void)height;
    if (!ascii_art || !filename) return 0;
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create output file %s\n", filename);
        return 0;
    }
    
    fprintf(fp, "%s", ascii_art);
    fclose(fp);
    
    printf("ASCII art saved to %s\n", filename);
    return 1;
}

void print_available_charsets(void) {
    printf("Available character sets:\n");
    for (int i = 0; i < NUM_ASCII_SETS; i++) {
        printf("  %d: %s - \"%s\"\n", i, ASCII_SETS[i].name, ASCII_SETS[i].chars);
    }
}
