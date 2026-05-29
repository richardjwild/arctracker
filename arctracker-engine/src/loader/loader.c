#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "loader.h"
#include "format_desktop_tracker.h"
#include "format_tracker.h"
#include "io/error.h"
#include "memory/heap.h"

#define NUM_FORMATS 2
#define FAILED_TO_READ_FILE (load_module_result_t) {\
    .file_read = false,\
    .recognised_format = false,\
    .module_loaded = false,\
    .module = NULL\
}
#define UNRECOGNISED_FORMAT (load_module_result_t) {\
    .file_read = true,\
    .recognised_format = false,\
    .module_loaded = false,\
    .module = NULL\
}
#define FAILED_TO_LOAD_MODULE (load_module_result_t) {\
    .file_read = true,\
    .recognised_format = true,\
    .module_loaded = false,\
    .module = NULL\
}
#define SUCCESSFULLY_LOADED(module_p) (load_module_result_t) {\
    .file_read = true,\
    .recognised_format = true,\
    .module_loaded = true,\
    .module = module_p\
}

static const char *READONLY = "r";
static const int OFFSET = 0;

mapped_file_t load_file(FILE *);
size_t file_size(int);
format_t *known_formats(void);

load_module_result_t load_module(const char *mod_filename)
{
    FILE *file_pointer = fopen(mod_filename, READONLY);
    if (file_pointer == NULL)
    {
        return FAILED_TO_READ_FILE;
    }
    const mapped_file_t file = load_file(file_pointer);
    fclose(file_pointer);
    if (has_error())
    {
        return FAILED_TO_READ_FILE;
    }
    const format_t *formats = known_formats();
    for (int i = 0; i < NUM_FORMATS; i++)
    {
        const format_t format = formats[i];
        if (format.is_this_format(file)) {
            module_t *module = format.read_module(file);
            munmap(file.addr, file.size);
            if (has_error())
            {
                module_destroy(module);
                return FAILED_TO_LOAD_MODULE;
            }
            return SUCCESSFULLY_LOADED(module);
        }
    }
    return UNRECOGNISED_FORMAT;
}

mapped_file_t load_file(FILE *file_pointer)
{
    mapped_file_t mapped_file = {
        .addr = NULL,
        .size = 0
    };
    const int file_descriptor = fileno(file_pointer);
    const size_t size = file_size(file_descriptor);
    if (has_error())
    {
        return mapped_file;
    }
    uint8_t *addr = mmap(NULL, size, PROT_READ, MAP_SHARED, file_descriptor, OFFSET);
    if (addr == MAP_FAILED)
    {
        system_error(errno);
    }
    else
    {
        mapped_file.size = size;
        mapped_file.addr = addr;
    }
    return mapped_file;
}

size_t file_size(const int file_descriptor)
{
    struct stat statbuf;
    if (fstat(file_descriptor, &statbuf) != 0)
    {
        system_error(errno);
        return 0;
    }
    return statbuf.st_size;
}

format_t *known_formats(void)
{
    static format_t formats[NUM_FORMATS];
    formats[0] = tracker_format();
    formats[1] = desktop_tracker_format();
    return formats;
}
