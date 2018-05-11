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

#include "arctracker.h"
#include "read_mod.h"
#include "config.h"

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

return_status read_file(
	void *p_modfile,
        long p_modsize,
        module_t *p_module,
	sample_t *p_samples)
{
	return_status retcode;
	void *chunk_address;
	long array_end = (long)p_modfile + p_modsize;

	/* get module information */

	retcode = search_tff(
		p_modfile,
		&chunk_address,
		array_end,
		MUSX_CHUNK,
		1);

	if (retcode == CHUNK_NOT_FOUND)	{
#ifdef DEVELOPING
		printf("MUSX chunk not found, looking for DskT chunk...\n");
#endif
		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			DSKT_CHUNK,
			1);

		if (retcode == CHUNK_NOT_FOUND) {
			fprintf(stderr,"File type not recognised\n");
			retcode = NOT_MODULE;
		} else {
			printf("File is DESKTOP TRACKER format.\n");
			if (chunk_address != p_modfile) {
				p_modfile = chunk_address;
			}
			p_module->format = DESKTOP_TRACKER;
			retcode = read_desktop_tracker_file(
				p_modfile,
				p_module,
				p_samples);
		}
	} else {
		printf("File is TRACKER format.\n");
		p_module->format = TRACKER;
		retcode = read_tracker_file(
			p_modfile,
			p_modsize,
			p_module,
			p_samples);
	}

	if (retcode == SUCCESS) {
		printf("Module name: %s\nAuthor: %s\n",
		       p_module->name,
		       p_module->author);
	}

	return (retcode);
}

return_status read_tracker_file(
	void *p_modfile,
	long p_modsize,
	module_t *p_module,
	sample_t *p_samples)
{
	return_status retcode;
	void *chunk_address;
	long array_end = (long)p_modfile + p_modsize;
	int num_patterns;
	int num_samples;

#ifdef DEVELOPING
	printf("Found MUSX chunk.\n");
#endif

	retcode = search_tff(
		p_modfile,
		&chunk_address,
		array_end,
		TINF_CHUNK,
		1);

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - TINF chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nchar(p_module->tracker_version, chunk_address+8, 4, true);
#ifdef DEVELOPING
		printf("Found TINF chunk.  Tracker version=%s\n", p_module->tracker_version);
#endif

		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			MVOX_CHUNK,
			1);
	}

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - MVOX chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nbytes(&(p_module->num_channels), chunk_address+8, 4);
#ifdef DEVELOPING
		printf("Found MVOX chunk.  Number of voices=%d\n", p_module->num_channels);
#endif

		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			STER_CHUNK,
			1);
	}

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - STER chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nchar(p_module->default_channel_stereo, chunk_address+8, MAX_CHANNELS, false);
#ifdef DEVELOPING
		printf(
			"Found STER chunk.  Default stereo positions:\n"
			"channel 1 = %d\n"
			"channel 2 = %d\n"
			"channel 3 = %d\n"
			"channel 4 = %d\n"
			"channel 5 = %d\n"
			"channel 6 = %d\n"
			"channel 7 = %d\n"
			"channel 8 = %d\n",
			p_module->default_channel_stereo[0],
			p_module->default_channel_stereo[1],
			p_module->default_channel_stereo[2],
			p_module->default_channel_stereo[3],
			p_module->default_channel_stereo[4],
			p_module->default_channel_stereo[5],
			p_module->default_channel_stereo[6],
			p_module->default_channel_stereo[7]);
#endif

		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			MNAM_CHUNK,
			1);
	}

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - MNAM chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nchar(p_module->name, chunk_address+8, MAX_LEN_TUNENAME, true);
#ifdef DEVELOPING
		printf("Found MNAM chunk.  Tune name = %s\n", p_module->name);
#endif

		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			ANAM_CHUNK,
			1);
	}

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - ANAM chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nchar(p_module->author, chunk_address+8, MAX_LEN_AUTHOR, true);
#ifdef DEVELOPING
		printf("Found ANAM chunk.  Author = %s\n", p_module->author);
#endif

		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			MLEN_CHUNK,
			1);
	}

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - MLEN chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nbytes(&(p_module->tune_length), chunk_address+8, 4);
#ifdef DEVELOPING
		printf("Found MLEN chunk.  Tune length = %d patterns\n", p_module->tune_length);
#endif

		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			PNUM_CHUNK,
			1);
	}

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - PNUM chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nbytes(&(p_module->num_patterns), chunk_address+8, 4);
#ifdef DEVELOPING
		printf("Found PNUM chunk.  Number of patterns = %d\n", p_module->num_patterns);
#endif

		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			PLEN_CHUNK,
			1);
	}

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - PLEN chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nchar(p_module->pattern_length, chunk_address+8, NUM_PATTERNS, false);
#ifdef DEVELOPING
		printf("Found PLEN chunk.  Pattern lengths:\n");

		for (i=0; i<NUM_PATTERNS; i++)
			printf("%d=%d ", i, p_module->pattern_length[i]);

		printf("\n");
#endif

		retcode = search_tff(
			p_modfile,
			&chunk_address,
			array_end,
			SEQU_CHUNK,
			1);
	}

	if (retcode == CHUNK_NOT_FOUND) {
		fprintf(stderr,"Modfile corrupt - SEQU chunk not found\n");
		retcode = FILE_CORRUPT;
	} else if (retcode == SUCCESS) {
		read_nchar(p_module->sequence, chunk_address+8, MAX_TUNELENGTH, false);
#ifdef DEVELOPING
		printf("Found SEQU chunk.  Sequence:\n");

		for (i=0; i<p_module->tune_length; i++)
			printf("%d ",p_module->sequence[i]);

		printf("\n");
#endif
	}

	/* get pattern information */

	if (retcode == SUCCESS)
		retcode = get_patterns(
			p_modfile,
			array_end,
			p_module->patterns,
			&num_patterns);

	/* get sample information */

	if (retcode == SUCCESS)
		retcode = get_samples(
			p_modfile,
			array_end,
			&num_samples,
			p_samples);

	if (retcode == SUCCESS)
		p_module->num_samples = num_samples;

	/* set default value for initial speed */
	if (retcode == SUCCESS)
		p_module->initial_speed = 6;

	return (retcode);
}

return_status read_desktop_tracker_file(
	void *p_modfile,
	module_t *p_module,
	sample_t *p_samples)
{
	return_status retcode = SUCCESS;
	void *tmp_ptr;
	long foo;
	int i;

#ifdef DEVELOPING
	char filename[2048];
	FILE *fp;
#endif

	read_nchar(p_module->name, p_modfile+4, MAX_LEN_TUNENAME_DSKT, true);
#ifdef DEVELOPING
	printf("Tune name is: %s\n", p_module->name);
#endif

	read_nchar(p_module->author, p_modfile+68, MAX_LEN_AUTHOR_DSKT, true);
#ifdef DEVELOPING
	printf("Author is: %s\n", p_module->author);
#endif

	read_nbytes(&(p_module->num_channels), p_modfile+136, 4);
#ifdef DEVELOPING
	printf("Number of voices = %d\n", p_module->num_channels);
#endif

	read_nbytes(&(p_module->tune_length), p_modfile+140, 4);
#ifdef DEVELOPING
	printf("Tune length = %d\n", p_module->tune_length);
#endif

	read_nchar(p_module->default_channel_stereo, p_modfile+144, MAX_CHANNELS_DSKT, false);
#ifdef DEVELOPING
	printf(
		"Default stereo positions: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		p_module->default_channel_stereo[0],
		p_module->default_channel_stereo[1],
		p_module->default_channel_stereo[2],
		p_module->default_channel_stereo[3],
		p_module->default_channel_stereo[4],
		p_module->default_channel_stereo[5],
		p_module->default_channel_stereo[6],
		p_module->default_channel_stereo[7],
		p_module->default_channel_stereo[8],
		p_module->default_channel_stereo[9],
		p_module->default_channel_stereo[10],
		p_module->default_channel_stereo[11],
		p_module->default_channel_stereo[12],
		p_module->default_channel_stereo[13],
		p_module->default_channel_stereo[14],
		p_module->default_channel_stereo[15]);
#endif

	read_nbytes(&(p_module->initial_speed), p_modfile+152, 4);
#ifdef DEVELOPING
	printf("Initial speed = %d\n", p_module->initial_speed);
#endif

	read_nbytes(&(p_module->num_patterns), p_modfile+160, 4);
#ifdef DEVELOPING
	printf("Number of patterns = %d\n", p_module->num_patterns);
#endif

	read_nbytes(&(p_module->num_samples), p_modfile+164, 4);
#ifdef DEVELOPING
	printf("Number of samples = %d\n", p_module->num_samples);
#endif

	read_nchar(p_module->sequence, p_modfile+168, p_module->tune_length, false);
#ifdef DEVELOPING
	printf("Sequence: ");
	for (i=0; i<p_module->tune_length; i++)
		printf("%d ",p_module->sequence[i]);
	printf("\n");
#endif

	tmp_ptr = p_modfile + 168 + (((p_module->tune_length + 3)>>2)<<2); /* align to word boundary */
	for (i=0; i<p_module->num_patterns; i++) {
		read_nbytes(&foo, tmp_ptr, 4);
		p_module->patterns[i] = p_modfile + foo;
#ifdef DEVELOPING
		printf(
			"Got pattern %d at offset %d, address %d, previous %d\n",
			i,
			foo,
			p_module->patterns[i],
			p_module->patterns[i]-p_module->patterns[i-1]);
#endif
		tmp_ptr += 4;
	}

#ifdef DEVELOPING
	printf("Pattern lengths:");
#endif
	for (i=0; i<p_module->num_patterns; i++) {
		p_module->pattern_length[i] = *(unsigned char *)tmp_ptr;
#ifdef DEVELOPING
		printf(" %d", p_module->pattern_length[i]);
#endif
		tmp_ptr++;
	}
#ifdef DEVELOPING
	printf("\n");
#endif

#ifdef DEVELOPING
	printf("Aligning pointer %d ", tmp_ptr);
#endif
	if (p_module->num_patterns % 4)
		tmp_ptr = tmp_ptr + (4 - (p_module->num_patterns % 4));
#ifdef DEVELOPING
	printf("to %d\n", tmp_ptr);
#endif

	for (i=0; i<p_module->num_samples; i++) {
		p_samples[i].transpose = 26 - *(unsigned char *)tmp_ptr++;
		p_samples[i].volume = *(unsigned char *)tmp_ptr;
		tmp_ptr+=3;
		read_nbytes(&(p_samples[i].period), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(p_samples[i].sustain_start), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(p_samples[i].sustain_length), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(p_samples[i].repeat_offset), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(p_samples[i].repeat_length), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nbytes(&(p_samples[i].sample_length), tmp_ptr, 4);
		tmp_ptr+=4;
		read_nchar(p_samples[i].name, tmp_ptr, MAX_LEN_SAMPLENAME_DSKT, true);
		tmp_ptr+=MAX_LEN_SAMPLENAME_DSKT;
		read_nbytes(&foo, tmp_ptr, 4);
		p_samples[i].sample_data = p_modfile + foo;
		tmp_ptr+=4;

#ifdef DEVELOPING
		printf(
			"%d.  %s: note=%d volume=%d period=%d sustain start=%d sustain end=%d "
			"repeat offset=%d repeat length=%d sample length=%d sample data=%d\n",
			i,
			p_samples[i].name,
			p_samples[i].note,
			p_samples[i].volume,
			p_samples[i].period,
			p_samples[i].sustain_start,
			p_samples[i].sustain_length,
			p_samples[i].repeat_offset,
			p_samples[i].repeat_length,
			p_samples[i].sample_length,
			p_samples[i].sample_data);
#endif

#ifdef DEVELOPING
		strcpy(filename, p_samples[i].name);
		strcat(filename, ".smp");
		fp = fopen(filename, "w");
		fwrite(p_samples[i].sample_data, 1, p_samples[i].sample_length, fp);
		fclose(fp);
#endif
	}

#ifdef DEVELOPING
	/* print out first pattern for debugging purposes */
	tmp_ptr = p_module->patterns[2];
	printf("attempting to print pattern data at address %d\n", tmp_ptr);
	for (i=0; i<p_module->pattern_length[2]; i++) {
		for (foo=0; foo<p_module->num_channels; foo++) {
			read_nbytes(&bar, tmp_ptr, 4);

			if (bar & 0xfe000000) {
				tmp_ptr+=4; /* is four effects */

				printf(
					"%s %s%s",
					notes_x[((bar & 4032) >> 6)],
					alphanum_x[(bar & 63)],
					alphanum_x[((bar & 0x1f000) >> 12) + 1]);

				read_nbytes(&bar, tmp_ptr, 4);
				tmp_ptr+=4;

				printf(" %8X | ", bar);
			} else {
				tmp_ptr+=4; /* is one effect */

				printf(
					"%s %s%s%X%X | ",
					notes_x[((bar & 4032) >> 6)],
					alphanum_x[(bar & 63)],
					alphanum_x[((bar & 0x1f000) >> 12) + 1],
					(bar & 0xf0000000) >> 28,
					(bar & 0xf000000) >> 24);
			}
		} /* end for foo */
		printf("\n");
	} /* end for i */
#endif

	return (retcode);
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

/* function get_patterns.                                                        *
 * Search for all PATT chunks in modfile and set pointers to each pattern's data */

return_status get_patterns(
	void *p_search_from,
	long p_array_end,
	void **p_patterns,
	int *p_num_patterns)
{
	return_status retcode;
	void *chunk_address;

	*p_num_patterns = 0;

	do {
		retcode = search_tff(p_search_from,
		                     &chunk_address,
				     p_array_end,
				     PATT_CHUNK,
				     (*p_num_patterns)+1);

		if (retcode == SUCCESS) {
			(*p_num_patterns)++;
			*(p_patterns++) = chunk_address+8;
#ifdef DEVELOPING
			printf("Found pattern %d at address %d\n", *p_num_patterns, chunk_address+8);
#endif
		}
	}
	while (retcode == SUCCESS);

	if (!*p_num_patterns) {
		fprintf(stderr,"Modfile corrupt - no patterns in module\n");
		retcode = NO_PATTERNS_IN_MODULE;
	} else
		retcode = SUCCESS;

	return (retcode);
}

/* function get_samples.                                                      *
 * Search for all SAMP chunks in modfile, obtain sample information and store */

return_status get_samples(
	void *p_search_from,
	long p_array_end,
	int  *p_samples_found,
	sample_t *p_samples)
{
	return_status retcode;
	int sample_chunks_found = 0;
	void *chunk_address;
	long chunk_length;

	*p_samples_found = 0;

	do {
		retcode = search_tff(
			p_search_from,
			&chunk_address,
			p_array_end,
			SAMP_CHUNK,
			sample_chunks_found+1);

		if (retcode == SUCCESS) {
			sample_chunks_found++;
			read_nbytes(&chunk_length, chunk_address+CHUNKSIZE, 4);
			retcode = get_sample_info(
				chunk_address,
				(long)chunk_address+chunk_length+8,
				p_samples);

			if (retcode == SUCCESS)
				p_samples++, (*p_samples_found)++; /* if sample corrupt (retcode!=SUCCESS) ignore it */
			else if (retcode == SAMPLE_INVALID) {
				/* if sample corrupt (retcode!=SUCCESS) ignore it */
				/* prevent a retcode != SUCCESS from propagating out of the function *
				 * where it will cause the player to reject the modfile              */
				retcode = SUCCESS;
			}
		}
	}
	while ((retcode != CHUNK_NOT_FOUND) && (sample_chunks_found < NUM_SAMPLES));

	if (!sample_chunks_found) {
		fprintf(stderr,"Mofile corrupt - no samples in module\n");
		retcode = NO_SAMPLES_IN_MODULE;
	} else if (retcode == CHUNK_NOT_FOUND)
		retcode = SUCCESS; /* make sure function returns SUCCESS if less than 36 sample chunks in modfile */

	return (retcode);
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
		read_nbytes(&p_sample->volume, chunk_address+8, 4);

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

	return (retcode);
}
