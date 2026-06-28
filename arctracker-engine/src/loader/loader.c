#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "loader.h"
#include <string.h>
#include "format_arctracker.h"
#include "format_desktop_tracker.h"
#include "format_tracker.h"
#include "audio_file/wav.h"
#include "io/error.h"
#include "memory/heap.h"

#define MAX_FILE_PATH_LENGTH 4096
#define NUM_FORMATS 3
#define FAILED_TO_READ_MODULE_FILE (load_module_result_t) {\
    .file_read = false,\
    .recognised_format = false,\
    .module_loaded = false,\
    .module = NULL\
}
#define UNRECOGNISED_MODULE_FORMAT (load_module_result_t) {\
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
#define SUCCESSFULLY_LOADED_MODULE(module_p) (load_module_result_t) {\
    .file_read = true,\
    .recognised_format = true,\
    .module_loaded = true,\
    .module = module_p\
}
#define FAILED_TO_READ_SAMPLE_FILE (load_sample_result_t) {\
    .file_read = false,\
    .file_valid = false,\
    .sample_length = 0,\
    .sample_data = NULL,\
}
#define SUCCESSFULLY_LOADED_SAMPLE(length, data) (load_sample_result_t) {\
    .file_read = true,\
    .file_valid = true,\
    .sample_length = length,\
    .sample_data = data,\
}

static const char *READONLY = "r";
static const int OFFSET = 0;

static mapped_file_t load_file(FILE *);
static size_t file_size(int);
static format_t *known_formats(void);

load_module_result_t load_module(const char *filename)
{
    FILE *file_pointer = fopen(filename, READONLY);
    if (file_pointer == NULL)
    {
        return FAILED_TO_READ_MODULE_FILE;
    }
    const mapped_file_t file = load_file(file_pointer);
    fclose(file_pointer);
    if (has_error())
    {
        return FAILED_TO_READ_MODULE_FILE;
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
            return SUCCESSFULLY_LOADED_MODULE(module);
        }
    }
    return UNRECOGNISED_MODULE_FORMAT;
}

load_sample_result_t load_sample(const char *filename)
{
    FILE *file_pointer = fopen(filename, READONLY);
    if (file_pointer == NULL)
    {
        return FAILED_TO_READ_SAMPLE_FILE;
    }
    const mapped_file_t file = load_file(file_pointer);
    const audio_t audio_data = wav_read_audio(file.addr, file.size);
    fclose(file_pointer);
    if (has_error())
    {
        return FAILED_TO_READ_SAMPLE_FILE;
    }
    munmap(file.addr, file.size);
    return SUCCESSFULLY_LOADED_SAMPLE(audio_data.frames, audio_data.audio_data);
}

bool save_module(const module_t *module, const char *filename, const format_t format)
{
    char tmp_filename[MAX_FILE_PATH_LENGTH];
    snprintf(tmp_filename, sizeof tmp_filename, "%s.tmp", filename);
    FILE *file_pointer = fopen(tmp_filename, "wb");
    if (file_pointer == NULL)
    {
        system_error(errno);
        return false;
    }
    bool ok = format.write_module(module, file_pointer);
    if (fclose(file_pointer) != 0) {
        system_error(errno);
        ok = false;
    }
    if (!ok) {
        remove(tmp_filename);
        return false;
    }
    if (rename(tmp_filename, filename) != 0) {
        system_error(errno);
        remove(tmp_filename);
        return false;
    }
    return ok;
}

static mapped_file_t load_file(FILE *file_pointer)
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

static size_t file_size(const int file_descriptor)
{
    struct stat statbuf;
    if (fstat(file_descriptor, &statbuf) != 0)
    {
        system_error(errno);
        return 0;
    }
    return statbuf.st_size;
}

static format_t *known_formats(void)
{
    static format_t formats[NUM_FORMATS];
    formats[0] = arctracker_format();
    formats[1] = tracker_format();
    formats[2] = desktop_tracker_format();
    return formats;
}
