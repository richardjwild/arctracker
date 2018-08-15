#ifndef ARCTRACKER_READ_MOD_H
#define ARCTRACKER_READ_MOD_H

#include <arctracker.h>

typedef struct {
    void *addr;
    size_t size;
} mapped_file_t;

typedef struct {
    bool (*is_this_format)(mapped_file_t);
    module_t (*read_module)(mapped_file_t);
} format_t;

module_t read_file(format_t *formats, int no_formats);

#endif //ARCTRACKER_READ_MOD_H
