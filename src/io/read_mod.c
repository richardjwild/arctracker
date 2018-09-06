#include "read_mod.h"
#include "configuration.h"
#include "console.h"
#include "error.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <config.h>
#include <memory/heap.h>

static const char *READONLY = "r";

mapped_file_t load_file(char *);

size_t file_size(int);

module_t read_file(format_t *formats, int no_formats)
{
    mapped_file_t file = load_file(configuration().mod_filename);
    for (int i = 0; i < no_formats; i++)
    {
        format_t format = formats[i];
        if (format.is_this_format(file))
        {
            module_t module = format.read_module(file);
            write_info(module);
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
