#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    int width;
    int height;
    int channels;
    uint8_t *data;
} Image;

Image* load_ppm_image(const char* filename);
Image* load_image(const char* filename);
void free_image(Image* img);
int get_file_format(const char* filename);
Image* create_image(int width, int height, int channels);

#define FORMAT_UNKNOWN 0
#define FORMAT_PPM     1
#define FORMAT_PGM     2

#endif
