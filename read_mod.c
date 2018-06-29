#include <sys/mman.h>
#include <sys/stat.h>
#include "arctracker.h"
#include "config.h"
#include "error.h"
#include "configuration.h"
#include "read_mod.h"
#include "heap.h"

mapped_file_t load_file(char *filename);

size_t file_size(int fd);

module_t read_file(module_format *formats, int no_formats)
{
    mapped_file_t file = load_file(configuration().mod_filename);

    for (int i = 0; i < no_formats; i++)
    {
        module_format format = formats[i];
        if (format.is_this_format(file))
        {
            module_t module = format.read_module(file);
            printf("File is %s format.\n", module.format_name);
            printf("Module name: %s\nAuthor: %s\n", module.name, module.author);
            return module;
        }
    }

    error("File type not recognised");
}

mapped_file_t load_file(char *filename)
{
    mapped_file_t mapped_file;
    FILE *fp = fopen(filename, READONLY);
    if (fp == NULL)
    {
        error("Cannot open file.");
    }
    int fd = fileno(fp);
    mapped_file.size = file_size(fd);
    mapped_file.addr = mmap(NULL, mapped_file.size, PROT_READ, MAP_SHARED, fd, 0);
    return mapped_file;
}

size_t file_size(int fd)
{
    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1)
    {
        system_error("Error reading file status");
    }
    return (size_t) statbuf.st_size;
}

void *search_tff(void *array_start, const long array_end, const void *to_find, long occurrence)
{
    while ((long) array_start <= (array_end - CHUNKSIZE))
    {
        if (memcmp(to_find, array_start, CHUNKSIZE) == 0)
        {
            occurrence -= 1;
        }
        if (occurrence == 0)
        {
            return array_start;
        }
        array_start += 1;
    }
    return CHUNK_NOT_FOUND;
}
