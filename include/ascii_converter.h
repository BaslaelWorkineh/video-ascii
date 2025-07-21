#ifndef ASCII_CONVERTER_H
#define ASCII_CONVERTER_H

#include "image_loader.h"

typedef struct {
    const char* chars;
    int length;
    const char* name;
} AsciiCharSet;

extern const AsciiCharSet ASCII_SETS[];
extern const int NUM_ASCII_SETS;

typedef struct {
    int char_set_index;
    int invert_brightness;
    double aspect_ratio_correction;
} AsciiConfig;

char* image_to_ascii(const Image* img, const AsciiConfig* config);
char brightness_to_ascii(uint8_t brightness, const AsciiCharSet* char_set, int invert);
void print_ascii_art(const char* ascii_art, int width, int height);
int save_ascii_to_file(const char* ascii_art, int width, int height, const char* filename);
AsciiConfig create_default_config(void);
void print_available_charsets(void);

#endif
