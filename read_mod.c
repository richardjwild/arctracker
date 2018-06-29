#include <sys/mman.h>
#include <sys/stat.h>
#include "arctracker.h"
#include "config.h"
#include "error.h"
#include "configuration.h"
#include "read_mod.h"
#include "heap.h"

typedef struct {
	void *addr;
	size_t size;
} mapped_file_t;

module_t read_tracker_file(mapped_file_t file);

module_t read_desktop_tracker_file(mapped_file_t file);

return_status search_tff(
        void *array_start,
        void **found_chunk_address,
        long array_end,
        void *to_find,
        long occurrence);

void *search_tff2(
		void *array_start,
		const long array_end,
		const void *to_find,
		long occurrence);

#define CHUNK_NOT_FOUND_2 NULL
#define SAMPLE_INVALID NULL

void read_nchar(
        char *p_output,
        void *p_input,
        int p_num_chars,
        bool p_null_term);

void get_patterns(void *p_search_from, long p_array_end, void **p_patterns);

int get_samples(void *array_start, long array_end, sample_t *samples);

sample_t* get_sample_info(void *array_start, long array_end);

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

module_t read_file()
{
    mapped_file_t file = load_file(configuration().mod_filename);
    return_status retcode;
    module_t module;
    void *chunk_address;
    long array_end = (long) file.addr + file.size;

    retcode = search_tff(
            file.addr,
            &chunk_address,
            array_end,
            MUSX_CHUNK,
            1);

    if (retcode == CHUNK_NOT_FOUND)
    {
        retcode = search_tff(
                file.addr,
                &chunk_address,
                array_end,
                DSKT_CHUNK,
                1);

        if (retcode == SUCCESS)
        {
            printf("File is DESKTOP TRACKER format.\n");
            if (chunk_address != file.addr)
            {
                file.addr = chunk_address;
            }
            module = read_desktop_tracker_file(file);
        }
        else
            error("File type not recognised");
    }
    else
    {
        printf("File is TRACKER format.\n");
        module = read_tracker_file(file);
    }

    printf("Module name: %s\nAuthor: %s\n", module.name, module.author);

    return module;
}

module_t read_tracker_file(mapped_file_t file)
{
	void *chunk_address;
	long array_end = (long) file.addr + file.size;
    module_t module = {
            .format = TRACKER,
            .initial_speed = 6,
            .samples = allocate_array(36, sizeof(sample_t))
    };

	if (search_tff(file.addr, &chunk_address, array_end, TINF_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - TINF chunk not found");

    read_nchar(module.tracker_version, chunk_address + 8, 4, true);

    if (search_tff(file.addr, &chunk_address, array_end, MVOX_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - MVOX chunk not found");

    memcpy(&module.num_channels, chunk_address + 8, 4);

    if (search_tff(file.addr, &chunk_address, array_end, STER_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - STER chunk not found");

    read_nchar(module.default_channel_stereo, chunk_address + 8, MAX_CHANNELS, false);

    if (search_tff(file.addr, &chunk_address, array_end, MNAM_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - MNAM chunk not found");

    read_nchar(module.name, chunk_address + 8, MAX_LEN_TUNENAME, true);

    if (search_tff(file.addr, &chunk_address, array_end, ANAM_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - ANAM chunk not found");

    read_nchar(module.author, chunk_address + 8, MAX_LEN_AUTHOR, true);

    if (search_tff(file.addr, &chunk_address, array_end, MLEN_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - MLEN chunk not found");

    memcpy(&module.tune_length, chunk_address + 8, 4);

    if (search_tff(file.addr, &chunk_address, array_end, PNUM_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - PNUM chunk not found");

    memcpy(&module.num_patterns, chunk_address + 8, 4);

    if (search_tff(file.addr, &chunk_address, array_end, PLEN_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - PLEN chunk not found");

    read_nchar(module.pattern_length, chunk_address + 8, NUM_PATTERNS, false);

    if (search_tff(file.addr, &chunk_address, array_end, SEQU_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - SEQU chunk not found");

    read_nchar(module.sequence, chunk_address + 8, MAX_TUNELENGTH, false);

    get_patterns(file.addr, array_end, module.patterns);
	module.num_samples = get_samples(file.addr, array_end, module.samples);

	return module;
}

module_t read_desktop_tracker_file(mapped_file_t file)
{
	void *tmp_ptr;
	long foo;
	int i;
    module_t module = {
            .format = DESKTOP_TRACKER,
            .initial_speed = 6
    };

	read_nchar(module.name, file.addr+4, MAX_LEN_TUNENAME_DSKT, true);

	read_nchar(module.author, file.addr+68, MAX_LEN_AUTHOR_DSKT, true);

	memcpy(&module.num_channels, file.addr+136, 4);

	memcpy(&module.tune_length, file.addr+140, 4);

	read_nchar(module.default_channel_stereo, file.addr+144, MAX_CHANNELS_DSKT, false);

	memcpy(&module.initial_speed, file.addr+152, 4);

	memcpy(&module.num_patterns, file.addr+160, 4);

	memcpy(&module.num_samples, file.addr+164, 4);

	read_nchar(module.sequence, file.addr+168, module.tune_length, false);

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
		read_nchar(module.samples[i].name, tmp_ptr, MAX_LEN_SAMPLENAME_DSKT, true);
		tmp_ptr+=MAX_LEN_SAMPLENAME_DSKT;
		memcpy(&foo, tmp_ptr, 4);
        module.samples[i].sample_data = file.addr + foo;
        module.samples[i].repeats = (module.samples[i].repeat_length != 0);
		tmp_ptr+=4;
	}

	return module;
}

/* function search_tff.                                            *
 * Search tracker modfile for the nth occurrence of a named chunk. */

return_status search_tff(
	void *array_start,
	void **found_chunk_address,
	long array_end,
	void *to_find,
	long occurrence)
{
	while ((long) array_start <= (array_end - CHUNKSIZE))
	{
		if (memcmp(to_find, array_start, CHUNKSIZE) == 0)
		{
			occurrence -= 1;
		}
		if (occurrence == 0)
		{
			*found_chunk_address = array_start;
			return SUCCESS;
		}
		array_start += 1;
	}
	return CHUNK_NOT_FOUND;
}

void *search_tff2(
		void *array_start,
		const long array_end,
		const void *to_find,
		long occurrence)
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
	return CHUNK_NOT_FOUND_2;
}

/* function read_nchar.                                                                               *
 * Reads n characters at the address pointed to by p_input and returns it as a null-terminated string *
 * pointed at by p_output.  p_output must point at an array large enough to hold n+1 chars!           */

void read_nchar(
	char *p_output,
	void *p_input,
	int p_num_chars,
	bool p_null_term)
{
	int i;

	for (i=0; i<p_num_chars; i++)
		p_output[i] = *(((char *)p_input) + i);

	if (p_null_term)
		p_output[p_num_chars] = '\0';
}

void get_patterns(void *p_search_from, long p_array_end, void **p_patterns)
{
    int pattern = 1;
    void *chunk_address;
    while ((chunk_address = search_tff2(p_search_from, p_array_end, PATT_CHUNK, pattern)) != CHUNK_NOT_FOUND_2)
    {
        pattern++;
        *(p_patterns++) = chunk_address + 8;
    }
    if (pattern == 1)
        error("Modfile corrupt - no patterns in module");
}

int get_samples(void *array_start, long array_end, sample_t *samples)
{
    int chunks_found = 0;
    int samples_found = 0;
    void *chunk_address;
    while ((chunk_address = search_tff2(array_start, array_end, SAMP_CHUNK, chunks_found + 1)) != CHUNK_NOT_FOUND_2
           && chunks_found < NUM_SAMPLES)
    {
        chunks_found++;
        long chunk_length;
        memcpy(&chunk_length, chunk_address + CHUNKSIZE, 4);
        sample_t *sample;
        if ((sample = get_sample_info(chunk_address, (long) chunk_address + chunk_length + 8)) != SAMPLE_INVALID)
        {
            memcpy(samples, sample, sizeof(sample_t));
            samples++;
            samples_found++;
        }
    }
    if (chunks_found == 0)
    {
        error("Modfile corrupt - no samples in module");
    }
    return samples_found;
}

sample_t* get_sample_info(void *array_start, long array_end)
{
	void *chunk_address;
	sample_t *sample = allocate_array(1, sizeof(sample_t));

    if ((chunk_address = search_tff2(array_start, array_end, SNAM_CHUNK, 1)) == CHUNK_NOT_FOUND_2)
        return SAMPLE_INVALID;
    else
        read_nchar(sample->name, chunk_address + 8, MAX_LEN_SAMPLENAME, true);

    if ((chunk_address = search_tff2(array_start, array_end, SVOL_CHUNK, 1)) == CHUNK_NOT_FOUND_2)
        return SAMPLE_INVALID;
    else
        memcpy(&sample->default_gain, chunk_address + 8, 4);

    if ((chunk_address = search_tff2(array_start, array_end, SLEN_CHUNK, 1)) == CHUNK_NOT_FOUND_2)
        return SAMPLE_INVALID;
    else
        memcpy(&sample->sample_length, chunk_address + 8, 4);

    if ((chunk_address = search_tff2(array_start, array_end, ROFS_CHUNK, 1)) == CHUNK_NOT_FOUND_2)
        return SAMPLE_INVALID;
    else
        memcpy(&sample->repeat_offset, chunk_address + 8, 4);

    if ((chunk_address = search_tff2(array_start, array_end, RLEN_CHUNK, 1)) == CHUNK_NOT_FOUND_2)
        return SAMPLE_INVALID;
    else
        memcpy(&sample->repeat_length, chunk_address + 8, 4);

    if ((chunk_address = search_tff2(array_start, array_end, SDAT_CHUNK, 1)) == CHUNK_NOT_FOUND_2)
        return SAMPLE_INVALID;
    else
        sample->sample_data = chunk_address + 8;

	// transpose all notes up an octave when playing a Tracker module
	// compensating for the greater chromatic range in a Desktop Tracker module
	sample->transpose = 12;
    sample->repeats = (sample->repeat_length != 2);

	return sample;
}
