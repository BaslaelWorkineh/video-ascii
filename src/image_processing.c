#include "image_processing.h"
#include <math.h>
#include <string.h>

Image* convert_to_grayscale(const Image* img) {
    if (!img || !img->data) return NULL;
    
    if (img->channels == 1) {
        Image* gray_img = create_image(img->width, img->height, 1);
        if (!gray_img) return NULL;
        
        memcpy(gray_img->data, img->data, img->width * img->height);
        return gray_img;
    }

    Image* gray_img = create_image(img->width, img->height, 1);
    if (!gray_img) return NULL;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int rgb_idx = (y * img->width + x) * img->channels;
            int gray_idx = y * img->width + x;
            
            uint8_t r = img->data[rgb_idx];
            uint8_t g = img->data[rgb_idx + 1];
            uint8_t b = img->data[rgb_idx + 2];
            
            
            uint8_t gray = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
            gray_img->data[gray_idx] = gray;
        }
    }
    
    return gray_img;
}

uint8_t get_pixel_brightness(const Image* img, int x, int y) {
    if (!img || !img->data || x < 0 || x >= img->width || y < 0 || y >= img->height) {
        return 0;
    }
    
    if (img->channels == 1) {
        return img->data[y * img->width + x];
    } else {
        int idx = (y * img->width + x) * img->channels;
        uint8_t r = img->data[idx];
        uint8_t g = img->data[idx + 1];
        uint8_t b = img->data[idx + 2];
        return (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
    }
}

uint8_t bilinear_interpolate(const Image* img, float x, float y, int channel) {
    int x1 = (int)floor(x);
    int y1 = (int)floor(y);
    int x2 = x1 + 1;
    int y2 = y1 + 1;
    
    x1 = (x1 < 0) ? 0 : (x1 >= img->width) ? img->width - 1 : x1;
    y1 = (y1 < 0) ? 0 : (y1 >= img->height) ? img->height - 1 : y1;
    x2 = (x2 < 0) ? 0 : (x2 >= img->width) ? img->width - 1 : x2;
    y2 = (y2 < 0) ? 0 : (y2 >= img->height) ? img->height - 1 : y2;
    
    float fx = x - x1;
    float fy = y - y1;
    
    uint8_t p11, p12, p21, p22;
    
    if (img->channels == 1) {
        p11 = img->data[y1 * img->width + x1];
        p12 = img->data[y2 * img->width + x1];
        p21 = img->data[y1 * img->width + x2];
        p22 = img->data[y2 * img->width + x2];
    } else {
        p11 = img->data[(y1 * img->width + x1) * img->channels + channel];
        p12 = img->data[(y2 * img->width + x1) * img->channels + channel];
        p21 = img->data[(y1 * img->width + x2) * img->channels + channel];
        p22 = img->data[(y2 * img->width + x2) * img->channels + channel];
    }
    
    float val = p11 * (1 - fx) * (1 - fy) +
                p21 * fx * (1 - fy) +
                p12 * (1 - fx) * fy +
                p22 * fx * fy;
    
    return (uint8_t)val;
}

Image* resize_image(const Image* img, int new_width, int new_height) {
    if (!img || !img->data || new_width <= 0 || new_height <= 0) return NULL;
    
    Image* resized = create_image(new_width, new_height, img->channels);
    if (!resized) return NULL;
    
    float x_ratio = (float)img->width / new_width;
    float y_ratio = (float)img->height / new_height;
    
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;
            
            for (int c = 0; c < img->channels; c++) {
                uint8_t pixel_val = bilinear_interpolate(img, src_x, src_y, c);
                int dst_idx = (y * new_width + x) * img->channels + c;
                resized->data[dst_idx] = pixel_val;
            }
        }
    }
    
    return resized;
}

Image* resize_image_aspect_ratio(const Image* img, int max_width, int max_height) {
    if (!img || !img->data || max_width <= 0 || max_height <= 0) return NULL;
    
    float width_ratio = (float)max_width / img->width;
    float height_ratio = (float)max_height / img->height;
    float scale = (width_ratio < height_ratio) ? width_ratio : height_ratio;
    
    int new_width = (int)(img->width * scale);
    int new_height = (int)(img->height * scale);
    
    if (new_width < 1) new_width = 1;
    if (new_height < 1) new_height = 1;
    
    return resize_image(img, new_width, new_height);
}
