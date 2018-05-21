/* Copyright (c) Richard Wild 2004, 2005                                   *
 *                                                                         *
 * This file is part of Arctracker.                                        *
 *                                                                         *
 * Arctracker is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * Arctracker is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with Arctracker; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MS 02111-1307 USA */

#include <sys/mman.h>
#include <sys/stat.h>
#include "arctracker.h"
#include "config.h"
#include "error.h"
#include "configuration.h"
#include "read_mod.h"
#include "heap.h"

char *notes_x[] = {"---",
	"C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
	"C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
	"C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3"};

char *alphanum_x[] = {"--",
	"00","01","02","03","04","05","06","07","08","09",
	"0A","0B","0C","0D","0E","0F","10","11","12","13",
	"14","15","16","17","18","19","1A","1B","1C","1D",
	"1E", "1F", "20", "21", "22", "23"};

/* function read_file.                                                                      *
 * Read tracker modfile that is loaded in memory, and obtain its details; store information *
 * in structures for easier subsequent processing                                           */

typedef struct {
	void *addr;
	size_t size;
} mapped_file_t;

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
	{
		error("Cannot open file.");
	}
	else
	{
		int fd = fileno(fp);
		mapped_file.size = file_size(fd);
		mapped_file.addr = mmap(NULL, mapped_file.size, PROT_READ, MAP_SHARED, fd, 0);
	}
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
            module = read_desktop_tracker_file(file.addr);
        }
        else
            error("File type not recognised");
    }
    else
    {
        printf("File is TRACKER format.\n");
        module = read_tracker_file(file.addr, file.size);
    }

    printf("Module name: %s\nAuthor: %s\n", module.name, module.author);

    return module;
}

module_t read_tracker_file(void *p_modfile, long p_modsize)
{
	void *chunk_address;
	long array_end = (long)p_modfile + p_modsize;
    module_t module = {
            .format = TRACKER,
            .initial_speed = 6,
            .samples = allocate_array(36, sizeof(sample_t))
    };

	if (search_tff(p_modfile, &chunk_address, array_end, TINF_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - TINF chunk not found");

    read_nchar(module.tracker_version, chunk_address + 8, 4, true);

    if (search_tff(p_modfile, &chunk_address, array_end, MVOX_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - MVOX chunk not found");

    read_nbytes(&module.num_channels, chunk_address + 8, 4);

    if (search_tff(p_modfile, &chunk_address, array_end, STER_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - STER chunk not found");

    read_nchar(module.default_channel_stereo, chunk_address + 8, MAX_CHANNELS, false);

    if (search_tff(p_modfile, &chunk_address, array_end, MNAM_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - MNAM chunk not found");

    read_nchar(module.name, chunk_address + 8, MAX_LEN_TUNENAME, true);

    if (search_tff(p_modfile, &chunk_address, array_end, ANAM_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - ANAM chunk not found");

    read_nchar(module.author, chunk_address + 8, MAX_LEN_AUTHOR, true);

    if (search_tff(p_modfile, &chunk_address, array_end, MLEN_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - MLEN chunk not found");

    read_nbytes(&module.tune_length, chunk_address + 8, 4);

    if (search_tff(p_modfile, &chunk_address, array_end, PNUM_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - PNUM chunk not found");

    read_nbytes(&module.num_patterns, chunk_address + 8, 4);

    if (search_tff(p_modfile, &chunk_address, array_end, PLEN_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - PLEN chunk not found");

    read_nchar(module.pattern_length, chunk_address + 8, NUM_PATTERNS, false);

    if (search_tff(p_modfile, &chunk_address, array_end, SEQU_CHUNK, 1) == CHUNK_NOT_FOUND)
		error("Modfile corrupt - SEQU chunk not found");

    read_nchar(module.sequence, chunk_address + 8, MAX_TUNELENGTH, false);

    get_patterns(p_modfile, array_end, module.patterns);
	module.num_samples = get_samples(p_modfile, array_end, module.samples);

	return module;
}

module_t read_desktop_tracker_file(void *p_modfile)
{
	return_status retcode = SUCCESS;
	void *tmp_ptr;
	long foo;
	int i;
    module_t module = {
            .format = DESKTOP_TRACKER,
            .initial_speed = 6
    };

	read_nchar(module.name, p_modfile+4, MAX_LEN_TUNENAME_DSKT, true);

	read_nchar(module.author, p_modfile+68, MAX_LEN_AUTHOR_DSKT, true);

	read_nbytes(&module.num_channels, p_modfile+136, 4);

	read_nbytes(&module.tune_length, p_modfile+140, 4);

	read_nchar(module.default_channel_stereo, p_modfile+144, MAX_CHANNELS_DSKT, false);

	read_nbytes(&module.initial_speed, p_modfile+152, 4);

	read_nbytes(&module.num_patterns, p_modfile+160, 4);

	read_nbytes(&module.num_samples, p_modfile+164, 4);

	read_nchar(module.sequence, p_modfile+168, module.tune_length, false);

	tmp_ptr = p_modfile + 168 + (((module.tune_length + 3)>>2)<<2); /* align to word boundary */
	for (i=0; i<module.num_patterns; i++)
	{
		read_nbytes(&foo, tmp_ptr, 4);
		module.patterns[i] = p_modfile + foo;
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
		read_nbytes(&(module.samples[i].period), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(module.samples[i].sustain_start), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(module.samples[i].sustain_length), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(module.samples[i].repeat_offset), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(module.samples[i].repeat_length), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(module.samples[i].sample_length), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nchar(module.samples[i].name, tmp_ptr, MAX_LEN_SAMPLENAME_DSKT, true);
		tmp_ptr+=MAX_LEN_SAMPLENAME_DSKT;
		read_nbytes(&foo, tmp_ptr, 4);
        module.samples[i].sample_data = p_modfile + foo;
        module.samples[i].repeats = (module.samples[i].repeat_length != 0);
		tmp_ptr+=4;
	}

	return module;
}

/* function search_tff.                                            *
 * Search tracker modfile for the nth occurrence of a named chunk. */

return_status search_tff(
	void *p_searchfrom,
	void **p_chunk_address,
	long p_array_end,
	char *p_chunk,
	long p_occurrence)
{
	return_status retcode = CHUNK_NOT_FOUND;
	char current_chunk[CHUNKSIZE+1] = "xxxx"; /* ensure that null terminator is already present */

	do {
		read_nchar(current_chunk, p_searchfrom, CHUNKSIZE, true);

		if (strcmp(p_chunk, current_chunk) == STRINGS_MATCH) {
			if (--p_occurrence == 0) {
				retcode          = SUCCESS;
				*p_chunk_address = p_searchfrom;
			} else
				p_searchfrom++;
		} else
			p_searchfrom++;
	}
	while ((retcode == CHUNK_NOT_FOUND) && ((long)p_searchfrom <= (p_array_end-CHUNKSIZE)));
#ifdef DEVELOPING
	if (retcode == CHUNK_NOT_FOUND)
		printf("Gave up at address %d\n", p_searchfrom);
#endif

	return (retcode);
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

/* function read_nbytes.                                                                                 *
 * Reads n bytes at the address pointed to by p_input and returns it in the long pointed at by p_output. */

void read_nbytes(
	long *p_output,
	void *p_input,
	int p_num_bytes)
{
	int i;
	*p_output = 0;

	for (i=0; i<p_num_bytes; i++) {
		*p_output = (*p_output | (*(((unsigned char *)p_input) + i) << (8*i)));
	}
}

void get_patterns(void *p_search_from, long p_array_end, void **p_patterns)
{
	int pattern = 1;
	void *chunk_address;
    while (search_tff(p_search_from, &chunk_address, p_array_end, PATT_CHUNK, pattern) == SUCCESS)
    {
        pattern++;
        *(p_patterns++) = chunk_address + 8;
    }
	if (pattern == 1)
        error("Modfile corrupt - no patterns in module");
}

int get_samples(void *p_search_from, long p_array_end, sample_t *p_samples)
{
    int chunks_found = 0;
    int samples_found = 0;
    void *chunk_address;
    while (search_tff(p_search_from, &chunk_address, p_array_end, SAMP_CHUNK, chunks_found + 1) == SUCCESS
           && chunks_found < NUM_SAMPLES)
    {
        chunks_found++;
        long chunk_length;
        read_nbytes(&chunk_length, chunk_address + CHUNKSIZE, 4);
        if (get_sample_info(chunk_address, (long) chunk_address + chunk_length + 8, p_samples) == SUCCESS)
        {
            p_samples++;
            samples_found++;
        }
    }
    if (chunks_found == 0)
    {
        error("Modfile corrupt - no samples in module");
    }
    return samples_found;
}

/* function get_sample_info.                      *
 * Obtain information about samples one at a time */

return_status get_sample_info(
	void *p_search_from,
	long p_array_end,
	sample_t *p_sample)
{
	void *chunk_address;
	return_status retcode;

	retcode = search_tff(
		p_search_from,
		&chunk_address,
		p_array_end,
		SNAM_CHUNK,
		1);

	// transpose all notes up an octave when playing a Tracker module
	// compensating for the greater chromatic range in a Desktop Tracker module
	p_sample->transpose = 12;

	if (retcode == SUCCESS) {
		read_nchar(p_sample->name, chunk_address+8, MAX_LEN_SAMPLENAME, true);

		retcode = search_tff(
			p_search_from,
			&chunk_address,
			p_array_end,
			SVOL_CHUNK,
			1);
	} else {
#ifdef DEVELOPING
		printf("Warning - sample %d corrupt - SNAM chunk not found\n", p_sample_number);
#endif
		retcode = SAMPLE_INVALID;
	}

	if (retcode == SUCCESS) {
		read_nbytes(&p_sample->default_gain, chunk_address+8, 4);

		retcode = search_tff(
			p_search_from,
			&chunk_address,
			p_array_end,
			SLEN_CHUNK,
			1);
	} else if (retcode == CHUNK_NOT_FOUND) {
#ifdef DEVELOPING
		printf("Warning - sample %d corrupt - SVOL chunk not found\n", p_sample_number);
#endif
		retcode = SAMPLE_INVALID;
	}

	if (retcode == SUCCESS) {
		read_nbytes(&p_sample->sample_length, chunk_address+8, 4);

		retcode = search_tff(
			p_search_from,
			&chunk_address,
			p_array_end,
			ROFS_CHUNK,
			1);
	} else if (retcode == CHUNK_NOT_FOUND) {
#ifdef DEVELOPING
		printf("Warning - sample %d corrupt - SLEN chunk not found\n", p_sample_number);
#endif
		retcode = SAMPLE_INVALID;
	}

	if (retcode == SUCCESS) {
		read_nbytes(&p_sample->repeat_offset, chunk_address+8, 4);

		retcode = search_tff(
			p_search_from,
			&chunk_address,
			p_array_end,
			RLEN_CHUNK,
			1);
	} else if (retcode == CHUNK_NOT_FOUND) {
#ifdef DEVELOPING
		printf("Warning - sample %d corrupt - ROFS chunk not found\n", p_sample_number);
#endif
		retcode = SAMPLE_INVALID;
	}

	if (retcode == SUCCESS) {
		read_nbytes(&p_sample->repeat_length, chunk_address+8, 4);

		retcode = search_tff(
			p_search_from,
			&chunk_address,
			p_array_end,
			SDAT_CHUNK,
			1);
	} else if (retcode == CHUNK_NOT_FOUND) {
#ifdef DEVELOPING
		printf("Warning - sample %d corrupt - RLEN chunk not found\n", p_sample_number);
#endif
		retcode = SAMPLE_INVALID;
	}

	if (retcode == SUCCESS)
		p_sample->sample_data = chunk_address+8;
	else if (retcode == CHUNK_NOT_FOUND) {
#ifdef DEVELOPING
		printf("Warning - sample %d corrupt - SDAT chunk not found\n", p_sample_number);
#endif
		retcode = SAMPLE_INVALID;
	}

#ifdef DEVELOPING
	if (retcode == SUCCESS && p_sample->sample_length) {
		printf(
			"Got sample %d, name=%s, vol=%d, length=%d, rpt offs=%d, rpt len=%d, data at address %d\n",
			p_sample_number,
			p_sample->name,
			p_sample->volume,
			p_sample->sample_length,
			p_sample->repeat_offset,
			p_sample->repeat_length,
			p_sample->sample_data);
	}
#endif

    p_sample->repeats = (p_sample->repeat_length != 2);

	return (retcode);
}
