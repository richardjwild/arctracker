#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include "loader.h"
#include "format_arctracker.h"
#include "format_desktop_tracker.h"
#include "format_soundtracker.h"
#include "format_tracker.h"
#include "messages.h"
#include "audio_file/wav.h"
#include "io/error.h"
#include "memory/heap.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/mman.h>
#endif

#define MAX_FILE_PATH_LENGTH 4096
#define NUM_FORMATS 4

static const load_module_result_t FAILED_TO_READ_MODULE_FILE = {
    .file_read = false,
    .recognised_format = false,
    .module_loaded = false,
    .module = NULL,
};

static const load_module_result_t UNRECOGNISED_MODULE_FORMAT = {
    .file_read = true,
    .recognised_format = false,
    .module_loaded = false,
    .module = NULL,
};

static const load_module_result_t FAILED_TO_LOAD_MODULE = {
    .file_read = true,
    .recognised_format = true,
    .module_loaded = false,
    .module = NULL,
};

static const load_sample_result_t FAILED_TO_READ_SAMPLE_FILE = {
    .file_read = false,
    .file_valid = false,
    .sample_length = 0,
    .sample_data = NULL,
};

static const char *READONLY = "r";
static const char *WRITE_BINARY = "wb";
static const int OFFSET = 0;

static mapped_file_t load_file(FILE *);
static void unmap_file(mapped_file_t);
static size_t file_size(int);
static format_t *known_formats(void);
static int16_t clamp_to_pcm16(float sample);
static load_module_result_t successfully_loaded_module(module_t *);
static load_sample_result_t successfully_loaded_sample(int length, float rate, float *data);

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
            unmap_file(file);
            if (module == NULL) return FAILED_TO_LOAD_MODULE;
            return successfully_loaded_module(module);
        }
    }
    unmap_file(file);
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
    unmap_file(file);
    if (has_error())
    {
        return FAILED_TO_READ_SAMPLE_FILE;
    }
    return successfully_loaded_sample(audio_data.frames, (float) audio_data.sample_rate, audio_data.audio_data);
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
        .sample_rate = (int) sample.sample_rate,
        .channels = 1,
        .bits_per_sample = 16,
        .frames = sample.sample_length,
        .pcm16 = (int16_t *) sample_data_pcm16,
    };
    const bool ok = wav_write_pcm16(filename, &to_write);
    deallocate(MAIN, sample_data_pcm16);
    return ok;
}

#ifdef _WIN32

#include <windows.h>
#include <io.h>

static mapped_file_t load_file(FILE *file_pointer)
{
    mapped_file_t mapped_file = {
        .addr = NULL,
        .size = 0
    };

    const int fd = _fileno(file_pointer);
    if (fd == -1) {
        system_error(errno);
        return mapped_file;
    }

    const intptr_t os_handle = _get_osfhandle(fd);
    if (os_handle == -1) {
        system_error(errno);
        return mapped_file;
    }

    HANDLE file_handle = (HANDLE)os_handle;

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle, &file_size)) {
        // Report GetLastError() using your Windows error handling
        return mapped_file;
    }

    if (file_size.QuadPart == 0) {
        error(FILE_EMPTY);
        return mapped_file;
    }

    if (file_size.QuadPart < 0 ||
        (uint64_t)file_size.QuadPart > SIZE_MAX) {
        error("File is too large to map");
        return mapped_file;
        }

    HANDLE mapping = CreateFileMapping(
        file_handle,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );

    if (mapping == NULL) {
        // Report GetLastError()
        return mapped_file;
    }

    void *addr = MapViewOfFile(
        mapping,
        FILE_MAP_READ,
        0,
        0,
        0
    );

    CloseHandle(mapping);

    if (addr == NULL) {
        // Report GetLastError()
        return mapped_file;
    }

    mapped_file.addr = addr;
    mapped_file.size = (size_t)file_size.QuadPart;

    return mapped_file;
}

static void unmap_file(mapped_file_t file) {
    UnmapViewOfFile(file.addr);
}

#else

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

static void unmap_file(mapped_file_t file) {
    munmap(file.addr, file.size);
}

#endif

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
    formats[3] = soundtracker_format();
    return formats;
}

static int16_t clamp_to_pcm16(const float sample)
{
    if (sample <= -1.0f) return INT16_MIN;
    if (sample >= 1.0f) return INT16_MAX;
    return (int16_t)(sample * INT16_MAX);
}

static load_module_result_t successfully_loaded_module(module_t *module)
{
    return (load_module_result_t) {
        .file_read = true,
        .recognised_format = true,
        .module_loaded = true,
        .module = module,
    };
}

static load_sample_result_t successfully_loaded_sample(const int length, const float rate, float *data)
{
    return (load_sample_result_t) {
        .file_read = true,
        .file_valid = true,
        .sample_length = length,
        .sample_rate = rate,
        .sample_data = data,
    };
}
