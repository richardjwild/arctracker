#ifndef ARCTRACKER_READ_MOD_H
#define ARCTRACKER_READ_MOD_H

#define READONLY "r"

#define CHUNK_NOT_FOUND NULL

typedef struct {
    void *addr;
    size_t size;
} mapped_file_t;

void *search_tff(void *array_start, long array_end, const void *to_find, long occurrence);

module_t read_file();

#endif //ARCTRACKER_READ_MOD_H
