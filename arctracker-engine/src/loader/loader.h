#ifndef ARCTRACKER_READ_MOD_H
#define ARCTRACKER_READ_MOD_H

#include <stdio.h>
#include "player/module.h"

typedef struct {
    uint8_t *addr;
    size_t size;
} mapped_file_t;

typedef struct {
    bool (*is_this_format)(mapped_file_t);
    module_t *(*read_module)(mapped_file_t);
    bool (*write_module)(const module_t *, FILE *);
} format_t;

typedef struct {
    bool file_read;
    bool recognised_format;
    bool module_loaded;
    module_t *module;
} load_module_result_t;

typedef struct {
    bool file_read;
    bool file_valid;
    int sample_length;
    float *sample_data;
} load_sample_result_t;

static const char *UNKNOWN_FORMAT = "UNKNOWN";

load_module_result_t load_module(const char *filename);

bool save_module(const module_t *module, const char *filename, format_t format);

load_sample_result_t load_sample(const char *filename);

#endif //ARCTRACKER_READ_MOD_H
