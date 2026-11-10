#ifndef ARCTRACKER_ENGINE_FILES_H
#define ARCTRACKER_ENGINE_FILES_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_FILE_PATH_LENGTH 4096
#define MODE_READ_BINARY "rb"
#define MODE_WRITE_BINARY "wb"

typedef struct {
    uint8_t *addr;
    size_t size;
} mapped_file_t;

FILE *file_open(const char *filename, const char *mode);

bool file_replace(const char *source, const char *destination);

bool file_remove(const char *filename);

mapped_file_t load_file(FILE *file_pointer);

void unmap_file(mapped_file_t file);

bool write_cc(FILE *file_handle, const char *, size_t);

bool write_u8(FILE *file_handle, uint8_t);

bool write_s8(FILE *file_handle, int8_t);

bool write_u16_le(FILE *file_handle, uint16_t);

bool write_s16_le(FILE *file_handle, int16_t);

bool write_u32_le(FILE *file_handle, uint32_t);

bool write_f32_le(FILE *file_handle, float);

#endif //ARCTRACKER_ENGINE_FILES_H
