#include <sys/mman.h>
#include <sys/stat.h>
#include "arctracker.h"
#include "config.h"
#include "error.h"
#include "configuration.h"
#include "read_mod.h"
#include "tracker_module.h"
#include "heap.h"

module_t read_desktop_tracker_file(mapped_file_t file);

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

module_t read_desktop_tracker_file(mapped_file_t file)
{
	void *tmp_ptr;
	long foo;
	int i;
    module_t module;

    memset(&module, 0, sizeof(module_t));
    module.format = DESKTOP_TRACKER;
    module.initial_speed = 6;

	strncpy(module.name, file.addr+4, MAX_LEN_TUNENAME_DSKT);

	strncpy(module.author, file.addr+68, MAX_LEN_AUTHOR_DSKT);

	memcpy(&module.num_channels, file.addr+136, 4);

	memcpy(&module.tune_length, file.addr+140, 4);

	memcpy(module.default_channel_stereo, file.addr+144, MAX_CHANNELS_DSKT);

	memcpy(&module.initial_speed, file.addr+152, 4);

	memcpy(&module.num_patterns, file.addr+160, 4);

	memcpy(&module.num_samples, file.addr+164, 4);

	memcpy(module.sequence, file.addr+168, (size_t) module.tune_length);

	tmp_ptr = file.addr + 168 + (((module.tune_length + 3)>>2)<<2); /* align to word boundary */
	for (i=0; i<module.num_patterns; i++)
	{
		memcpy(&foo, tmp_ptr, 4);
		module.patterns[i] = file.addr + foo;
		tmp_ptr += 4;
	}

	for (i=0; i<module.num_patterns; i++)
	{
		module.pattern_length[i] = *(unsigned char *)tmp_ptr;
		tmp_ptr++;
	}

	if (module.num_patterns % 4)
		tmp_ptr = tmp_ptr + (4 - (module.num_patterns % 4));

	module.samples = allocate_array(module.num_samples, sizeof(sample_t));
	memset(module.samples, 0, sizeof(sample_t) * module.num_samples);
	for (i=0; i<module.num_samples; i++)
	{
        module.samples[i].transpose = 26 - *(unsigned char *)tmp_ptr++;
		unsigned char sample_volume = *(unsigned char *)tmp_ptr;
        module.samples[i].default_gain = (sample_volume * 2) + 1;
		tmp_ptr+=3;
		memcpy(&(module.samples[i].period), tmp_ptr, 4);
		tmp_ptr+=4;
		memcpy(&(module.samples[i].sustain_start), tmp_ptr, 4);
		tmp_ptr+=4;
		memcpy(&(module.samples[i].sustain_length), tmp_ptr, 4);
		tmp_ptr+=4;
		memcpy(&(module.samples[i].repeat_offset), tmp_ptr, 4);
		tmp_ptr+=4;
		memcpy(&(module.samples[i].repeat_length), tmp_ptr, 4);
		tmp_ptr+=4;
		memcpy(&(module.samples[i].sample_length), tmp_ptr, 4);
		tmp_ptr+=4;
		strncpy(module.samples[i].name, tmp_ptr, MAX_LEN_SAMPLENAME_DSKT);
		tmp_ptr+=MAX_LEN_SAMPLENAME_DSKT;
		memcpy(&foo, tmp_ptr, 4);
        module.samples[i].sample_data = file.addr + foo;
        module.samples[i].repeats = (module.samples[i].repeat_length != 0);
		tmp_ptr+=4;
	}

	return module;
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
