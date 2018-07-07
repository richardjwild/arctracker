#include <sys/mman.h>
#include <sys/stat.h>
#include "arctracker.h"
#include "config.h"
#include "error.h"
#include "configuration.h"
#include "read_mod.h"
#include "heap.h"

mapped_file_t load_file(char *filename);

size_t file_size(int file_descriptor);

module_t read_file(format_t *formats, int no_formats)
{
    mapped_file_t file = load_file(configuration().mod_filename);

    for (int i = 0; i < no_formats; i++)
    {
        format_t format = formats[i];
        if (format.is_this_format(file))
        {
            module_t module = format.read_module(file);
            printf("File is %s format.\n", module.format);
            printf("Module name: %s\nAuthor: %s\n", module.name, module.author);
            return module;
        }
    }

    error("File type not recognised");
}

mapped_file_t load_file(char *filename)
{
    mapped_file_t file;
    FILE *file_pointer = fopen(filename, READONLY);
    if (file_pointer == NULL)
    {
        error("Cannot open file.");
    }
    int file_descriptor = fileno(file_pointer);
    file.size = file_size(file_descriptor);
    file.addr = mmap(NULL, file.size, PROT_READ, MAP_SHARED, file_descriptor, 0);
    return file;
}

size_t file_size(int file_descriptor)
{
    struct stat statbuf;
    if (fstat(file_descriptor, &statbuf) == -1)
    {
        system_error("Error reading file status");
    }
    return (size_t) statbuf.st_size;
}

void *search_tff(void *array_start, const long array_end, const void *to_find, long occurrence)
{
    while ((occurrence >= 1) && (long) array_start <= (array_end - CHUNK_SIZE))
    {
        if (memcmp(to_find, array_start, CHUNK_SIZE) == 0)
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
