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
#include "config.h"
#include "error.h"
#include "configuration.h"

/* function load_file.                                    *
 * Allocate memory and load modfile into the memory space */

return_status load_file (void **p_array_ptr, long *p_bytes_loaded)
{
	return_status retcode = SUCCESS;
	FILE *fp;
	long bytes_read = 0;

	/* open the file for reading */
	fp = fopen(configuration().mod_filename, READONLY);

	if (fp == NULL) {
		fprintf(stderr,"Cannot open file.\n");
		retcode = BAD_FILE;
	} else {

#ifdef DEVELOPING
	printf("Successfully opened file for reading.\n");
#endif

		do {
			if ((*p_array_ptr = realloc(*p_array_ptr, (*p_bytes_loaded)+ARRAY_CHUNK_SIZE)) != NULL) {
				bytes_read = (long)fread((*p_array_ptr + *p_bytes_loaded), 1, ARRAY_CHUNK_SIZE, fp);
				(*p_bytes_loaded) += bytes_read;
			} else {
				fprintf(stderr, "Failed to allocate memory.\n");
				retcode = MEMORY_FAILURE;
			}
		}
		while ((retcode == SUCCESS) && (bytes_read == ARRAY_CHUNK_SIZE));

		fclose(fp);
	}

#ifdef DEVELOPING
	if (retcode == SUCCESS)
		printf("File successfully loaded at address %d, bytes loaded=%d\n", *p_array_ptr, *p_bytes_loaded);
#endif

	return (retcode);
}
