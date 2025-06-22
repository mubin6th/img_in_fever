#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "colors.h"

#include "../include/argparse.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

typedef struct _Args {
    const char *input_path;
    const char *output_path;
    int is_version;
} Args;

typedef struct _Image {
    uint8_t *data;
    int width;
    int height;
    int bytes;
} Image;

const int EXIT_ERR = 255;
const char *const VERSION = "v1.0.0-dev";

void getWritePath(const char *path, char *out);
void loadImage(Image *self, const char *path);
void writeImage(Image *self, const char *path);
void freeImage(Image *self);
// colors must be RGB compatible.
uint32_t squaredEuclidianDistance(const uint8_t *color1,
                                  const uint8_t *color2);
// color is 24 bit divided across 3 8bit values.
uint8_t *getClosestColor(uint8_t *color, const uint8_t *list,
                         size_t list_size);
void quantizeImage(Image *img, const uint8_t *color_list,
                   size_t color_list_size);

int main(int argc, const char **argv) {
    Args args = {
        .input_path = NULL,
        .output_path = NULL,
        .is_version = 0
    };

    const char *const usages[] = {
        "iif [arguments] [parameters]",
        NULL
    };

    struct argparse_option arg_options[] = {
        OPT_STRING('i', "input", &args.input_path,
                   "path to the image.", NULL, 0, 0),
        OPT_STRING('o', "output", &args.output_path,
                   "provide an output path.", NULL, 0, 0),
        OPT_BOOLEAN('v', "version", &args.is_version,
                    "print version and exit.", NULL, 0, 0),
        OPT_HELP(),
        OPT_END(),
    };

    struct argparse arg_parser;
    argparse_init(&arg_parser, arg_options, usages, 0);
    argparse_describe(&arg_parser,
                      "A program to convert any image "
                      "into a nice warm image.", NULL);
    argparse_parse(&arg_parser, argc, argv);

    if (argc < 2) {
        argparse_help_cb_no_exit(&arg_parser, arg_options);
        return EXIT_ERR;
    }

    if (args.is_version) {
        fprintf(stdout, "iif %s\n", VERSION);
        return 0;
    }

    if (args.input_path == NULL) {
        fprintf(stderr, "error: no image provided.\n");
        return EXIT_ERR;
    }

    Image img;
    loadImage(&img, args.input_path);

    if (img.data == NULL) {
        fprintf(stderr, "error: failed to load image.\n");
        return EXIT_ERR;
    }

    fprintf(stdout,
            "Image spec:\n"
            "    dimensions: %dx%d\n"
            "    color channels: %d\n",
            img.width, img.height, img.bytes);

    quantizeImage(&img, colors, colors_len);

    char write_path[1 << 7];
    getWritePath(args.input_path, write_path);

    writeImage(&img, write_path);
    freeImage(&img);
    return 0;
}

void getWritePath(const char *path, char *out) {
    char path_separator =
#ifdef _WIN32
    '\\'
#else
    '/'
#endif
    ;

    size_t ext_idx = 0;
    size_t name_idx;

    size_t path_size = strlen(path);

    for (size_t i = path_size - 1; i >= 0; i--) {
        if (path[i] == '.' && ext_idx == 0) {
            ext_idx = i + 1;
        }
        else if (path[i] == path_separator) {
            name_idx = i + 1;
            break;
        }
    }

    for (size_t i = name_idx; i < ext_idx - 1; i++) {
        *out = path[i];
        out++;
    }

    *out = '\0';

    strcat(out, "_by_iif");
    strcat(out, &path[ext_idx - 1]);
}

void loadImage(Image *self, const char *path) {
    self->data = stbi_load(path,
                           &self->width, &self->height,
                           &self->bytes, 0);
}

void writeImage(Image *self, const char *path) {
    size_t path_size = strlen(path);
    size_t k = 0;

    for (size_t i = path_size - 1; i >= 0; i--) {
        if (path[i] == '.') {
            k = i + 1;
            break;
        }
    }

    if (!k) {
        return;
    }

    const char *type = &path[k];

    if (strcmp(type, "jpg") == 0 || strcmp(type, "jpeg") == 0) {
        // if quality is 100 then the filesize tends to be
        // much bigger than original.
        stbi_write_jpg(path, self->width, self->height, self->bytes,
                       self->data, 90);
    }
    else if (strcmp(type, "png") == 0) {
        stbi_write_png(path, self->width, self->height, self->bytes,
                       self->data, self->width * self->bytes);
    }
    else if (strcmp(type, "bmp") == 0) {
        stbi_write_bmp(path, self->width, self->height, self->bytes,
                       self->data);
    }
    else if (strcmp(type, "tga") == 0) {
        stbi_write_tga(path, self->width, self->height, self->bytes,
                       self->data);
    }
}

void freeImage(Image *self) {
    free(self->data);
}

uint32_t squaredEuclidianDistance(const uint8_t *color1,
                                  const uint8_t *color2)
{
    return (color1[0] - color2[0]) * (color1[0] - color2[0]) +
           (color1[1] - color2[1]) * (color1[1] - color2[1]) +
           (color1[2] - color2[2]) * (color1[2] - color2[2]);
}

uint8_t *getClosestColor(uint8_t *color, const uint8_t *list,
                         size_t list_size)
{
    const uint8_t *out = list;
    uint32_t least_distance = UINT32_MAX;
    uint32_t d;

    for (size_t i = 0; i < list_size; i += 3) {
        d = squaredEuclidianDistance(color, &list[i]);
        if (d > least_distance) {
            continue;
        }

        out = &list[i];
        least_distance = d;
    }

    return (uint8_t *)out;
}

void quantizeImage(Image *img, const uint8_t *color_list,
                   size_t color_list_size)
{
    // the quantizer doesn't work for [channel < 3] images.
    if (img->bytes < 3) {
        return;
    }

    fprintf(stdout, "\n");

    uint8_t *p = img->data;
    size_t grid_size = img->width * img->height * img->bytes;

    for (size_t i = 0; i < grid_size;
         i += img->bytes, p += img->bytes)
    {
        uint8_t *result = getClosestColor(p, color_list,
                                          color_list_size);
        p[0] = result[0];
        p[1] = result[1];
        p[2] = result[2];

        if (i % 3000 == 0) {
            // i + img->bytes because I've completed a the
            // calculation on a pixel when printing.
            fprintf(stdout, "progress: %lu/%lu (%.0lf%%)\r",
                    i + img->bytes,
                    grid_size, (double)i / grid_size * 100);
        }
    }
}
