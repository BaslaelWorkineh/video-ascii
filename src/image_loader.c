#define _GNU_SOURCE
#include "image_loader.h"
#include <string.h>
#include <ctype.h>
#include <strings.h>

Image* create_image(int width, int height, int channels) {
    Image* img = malloc(sizeof(Image));
    if (!img) return NULL;
    
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = malloc(width * height * channels);
    
    if (!img->data) {
        free(img);
        return NULL;
    }
    
    return img;
}

void free_image(Image* img) {
    if (img) {
        if (img->data) {
            free(img->data);
        }
        free(img);
    }
}

void skip_comments(FILE* fp) {
    int c;
    while ((c = fgetc(fp)) == '#') {
        while ((c = fgetc(fp)) != '\n' && c != EOF);
    }
    ungetc(c, fp);
}

Image* load_ppm_image(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    char magic[3];
    if (fread(magic, 1, 2, fp) != 2) {
        fprintf(stderr, "Error: Cannot read magic number\n");
        fclose(fp);
        return NULL;
    }
    magic[2] = '\0';
    
    if (strcmp(magic, "P3") != 0 && strcmp(magic, "P6") != 0) {
        fprintf(stderr, "Error: Not a valid PPM file (P3 or P6)\n");
        fclose(fp);
        return NULL;
    }
    
    int is_binary = (strcmp(magic, "P6") == 0);
    
    skip_comments(fp);
    
    int width, height, max_val;
    if (fscanf(fp, "%d %d %d", &width, &height, &max_val) != 3) {
        fprintf(stderr, "Error: Invalid PPM header\n");
        fclose(fp);
        return NULL;
    }
    
    if (max_val != 255) {
        fprintf(stderr, "Error: Only 8-bit PPM files are supported\n");
        fclose(fp);
        return NULL;
    }
    
    fgetc(fp);
    
    Image* img = create_image(width, height, 3);
    if (!img) {
        fprintf(stderr, "Error: Cannot allocate memory for image\n");
        fclose(fp);
        return NULL;
    }
    
    if (is_binary) {
        if (fread(img->data, 1, width * height * 3, fp) != (size_t)(width * height * 3)) {
            fprintf(stderr, "Error: Cannot read image data\n");
            free_image(img);
            fclose(fp);
            return NULL;
        }
    } else {
        for (int i = 0; i < width * height * 3; i++) {
            int val;
            if (fscanf(fp, "%d", &val) != 1) {
                fprintf(stderr, "Error: Cannot read pixel data\n");
                free_image(img);
                fclose(fp);
                return NULL;
            }
            img->data[i] = (uint8_t)val;
        }
    }
    
    fclose(fp);
    return img;
}

int get_file_format(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return FORMAT_UNKNOWN;
    
    if (strcasecmp(ext, ".ppm") == 0) return FORMAT_PPM;
    if (strcasecmp(ext, ".pgm") == 0) return FORMAT_PGM;
    
    return FORMAT_UNKNOWN;
}

Image* load_image(const char* filename) {
    int format = get_file_format(filename);
    
    switch (format) {
        case FORMAT_PPM:
        case FORMAT_PGM:
            return load_ppm_image(filename);
        default:
            fprintf(stderr, "Error: Unsupported file format. Supported: .ppm, .pgm\n");
            return NULL;
    }
}
