#include <sys/mman.h>
#include <sys/stat.h>
#include "arctracker.h"
#include "config.h"
#include "error.h"
#include "configuration.h"
#include "read_mod.h"
#include "heap.h"
#include "tracker_module.h"
#include "desktop_tracker_module.h"

size_t file_size(int fd)
{
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1)
		system_error("Error reading file status");
	return (size_t) statbuf.st_size;
}

mapped_file_t load_file(char *filename)
{
	mapped_file_t mapped_file;
	FILE *fp = fopen(filename, READONLY);
	if (fp == NULL)
		error("Cannot open file.");
    int fd = fileno(fp);
    mapped_file.size = file_size(fd);
    mapped_file.addr = mmap(NULL, mapped_file.size, PROT_READ, MAP_SHARED, fd, 0);
	return mapped_file;
}

module_t print_details(module_t module)
{
    printf("Module name: %s\nAuthor: %s\n", module.name, module.author);
    return module;
}

module_t read_file()
{
    mapped_file_t file = load_file(configuration().mod_filename);
    long array_end = (long) file.addr + file.size;

    if (search_tff(file.addr, array_end, MUSX_CHUNK, 1) != CHUNK_NOT_FOUND)
    {
        printf("File is TRACKER format.\n");
        return print_details(read_tracker_file(file));
    }
    else
    {
        void *chunk_address;
        if ((chunk_address = search_tff(file.addr, array_end, DSKT_CHUNK, 1)) == CHUNK_NOT_FOUND)
            error("File type not recognised");

        printf("File is DESKTOP TRACKER format.\n");
        file.addr = chunk_address;
        return print_details(read_desktop_tracker_file(file));
    }
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
