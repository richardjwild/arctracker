#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "loader.h"
#include "format_arctracker.h"
#include "format_desktop_tracker.h"
#include "format_tracker.h"
#include "messages.h"
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
static const char *WRITE_BINARY = "wb";
static const int OFFSET = 0;

static mapped_file_t load_file(FILE *);
static size_t file_size(int);
static format_t *known_formats(void);
static int16_t clamp_to_pcm16(float sample);

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
            if (module == NULL) return FAILED_TO_LOAD_MODULE;
            return SUCCESSFULLY_LOADED_MODULE(module);
        }
    }
    munmap(file.addr, file.size);
    return UNRECOGNISED_MODULE_FORMAT;
}

bool save_module(const module_t *module, const char *filename, const format_t format)
{
    if (format.write_module == NULL)
    {
        error(SAVE_MODULE_NOT_SUPPORTED);
        return false;
    }
    char tmp_filename[MAX_FILE_PATH_LENGTH];
    snprintf(tmp_filename, sizeof tmp_filename, "%s.tmp", filename);
    FILE *file_pointer = fopen(tmp_filename, WRITE_BINARY);
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

load_sample_result_t load_sample(const char *filename)
{
    FILE *file_pointer = fopen(filename, READONLY);
    if (file_pointer == NULL)
    {
        return FAILED_TO_READ_SAMPLE_FILE;
    }
    const mapped_file_t file = load_file(file_pointer);
    fclose(file_pointer);
    if (has_error())
    {
        return FAILED_TO_READ_SAMPLE_FILE;
    }
    const audio_t audio_data = wav_read_audio(file.addr, file.size);
    munmap(file.addr, file.size);
    if (has_error())
    {
        return FAILED_TO_READ_SAMPLE_FILE;
    }
    return SUCCESSFULLY_LOADED_SAMPLE(audio_data.frames, audio_data.audio_data);
}

bool export_sample(const module_t *module, const int instrument_index, const char *filename)
{
    const int sample_index = module->instruments[instrument_index].sample_index;
    const sample_t sample = module->samples[sample_index];
    uint8_t *sample_data_pcm16 = allocate_array(MAIN, sizeof(uint8_t) * 2, sample.sample_length);
    if (sample_data_pcm16 == NULL)
    {
        error(MEMORY_ALLOCATION_FAILED);
        return false;
    }
    int byte_offset = 0;
    for (int frame = 0; frame < sample.sample_length; frame++)
    {
        const int16_t pcm = clamp_to_pcm16(sample.sample_data[frame]);
        sample_data_pcm16[byte_offset++] = pcm & 0xff;
        sample_data_pcm16[byte_offset++] = pcm >> 8;
    }
    const wav_pcm16_t to_write = {
        .sample_rate = 44100,
        .channels = 1,
        .bits_per_sample = 16,
        .frames = sample.sample_length,
        .pcm16 = (int16_t *) sample_data_pcm16,
    };
    const bool ok = wav_write_pcm16(filename, &to_write);
    deallocate(MAIN, sample_data_pcm16);
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
    if (size == 0)
    {
        error(FILE_EMPTY);
    }
    if (has_error())
    {
        return mapped_file;
    }
    uint8_t *addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, file_descriptor, OFFSET);
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

static int16_t clamp_to_pcm16(const float sample)
{
    if (sample <= -1.0f) return INT16_MIN;
    if (sample >= 1.0f) return INT16_MAX;
    return (int16_t)(sample * INT16_MAX);
}

