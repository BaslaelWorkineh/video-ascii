#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include "image_loader.h"

Image* convert_to_grayscale(const Image* img);
Image* resize_image(const Image* img, int new_width, int new_height);
Image* resize_image_aspect_ratio(const Image* img, int max_width, int max_height);
uint8_t get_pixel_brightness(const Image* img, int x, int y);
uint8_t bilinear_interpolate(const Image* img, float x, float y, int channel);

#endif
