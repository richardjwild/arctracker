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

/* function get_arguments.                     *
 * Get the arguments passed to the application */

return_status get_arguments(
	int p_argc,
	char *p_argv[],
	program_arguments *p_args)
{
	return_status retcode = SUCCESS;
        int i;

	/* defaults */
	p_args->volume = 128;
	p_args->pianola = NO;
	p_args->api = NOT_SPECIFIED;
	p_args->mod_filename = "";
	p_args->loop_forever = NO;

        /* ignore first argument (i=1) as this is the executable name ("arctracker") */

	if (p_argc > 1) {
		for (i=1; i<p_argc; i++) {
			if (retcode == SUCCESS) {
				if (*(p_argv[i]) != '-' && ((p_argc - i) > 1)) {
					fprintf(stderr,"Modfile must be last argument\n");
					retcode = BAD_ARGUMENTS;
				} else if (*(p_argv[i]) == '-') {
					if (strcmp(p_argv[i], ARG_PIANOLA) == 0) {
						p_args->pianola = YES;
					} else if (strcmp(p_argv[i], ARG_LOOP) == 0) {
						p_args->loop_forever = YES;
					} else if (strcmp(p_argv[i], ARG_OSS) == 0) {
						if (p_args->api == NOT_SPECIFIED)
							p_args->api = OSS;
						else {
							fprintf(stderr, "Cannot specify output API more than once!\n");
							retcode = BAD_ARGUMENTS;
						}
					} else if (strcmp(p_argv[i], ARG_ALSA) == 0) {
#ifdef HAVE_LIBASOUND
						if (p_args->api == NOT_SPECIFIED)
							p_args->api = ALSA;
						else {
							fprintf(stderr, "Cannot specify output API more than once!\n");
							retcode = BAD_ARGUMENTS;
						}
#else
						fprintf(stderr, "ALSA sound output not available\n");
						retcode = API_NOT_AVAILABLE;
#endif
					} else if (strncmp(p_argv[i], ARG_VOLUME, strlen(ARG_VOLUME)) == 0) {
						p_argv[i]+=strlen(ARG_VOLUME);
						p_args->volume = atoi(p_argv[i]);
						if (p_args->volume < 1 || p_args->volume > 255) {
							fprintf(stderr, "Volume must be between 1 and 255\n");
							retcode = BAD_ARGUMENTS;
						}
					} else {
						fprintf(stderr,"Unknown argument %s\n", p_argv[i]);
						retcode = BAD_ARGUMENTS;
					}
				} else {
#ifdef DEVELOPING
					printf("Modfile is %s\n", p_argv[i]);
#endif
					p_args->mod_filename = p_argv[i];
				}
			}
		}

		if (!(*p_args->mod_filename) && (retcode != BAD_ARGUMENTS) && (retcode != API_NOT_AVAILABLE)) {
			fprintf(stderr,"Usage: arctracker [--loop] [--pianola] [--oss|--alsa] [--volume=<volume>] <modfile>\n");
			retcode = BAD_ARGUMENTS;
		}
	} else {
		fprintf(stderr,"Usage: arctracker [--loop] [--pianola] [--oss|--alsa] [--volume=<volume>] <modfile>\n");
		retcode = BAD_ARGUMENTS;
	}

	if (p_args->api == NOT_SPECIFIED)
		p_args->api = OSS;

	return (retcode);
}

/* function load_file.                                    *
 * Allocate memory and load modfile into the memory space */

return_status load_file (
	char *p_filename,
	void **p_array_ptr,
	long *p_bytes_loaded)
{
	return_status retcode = SUCCESS;
	FILE *fp;
	long bytes_read = 0;

	/* open the file for reading */
	fp = fopen(p_filename, READONLY);

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

#ifdef HAVE_LIBASOUND
/* function initialise_alsa.
 * Open the audio device for output using the ALSA api, and set *
 * the device parameters (format, channels, sample rate)        */

return_status initialise_alsa (
	snd_pcm_t **p_pb_handle,
	long *p_sample_rate)
{
	return_status retcode = SUCCESS;
	int err;
	unsigned int tmp_srate;
	snd_pcm_hw_params_t *hw_params;

	tmp_srate = (unsigned int)*p_sample_rate;

	if ((err = snd_pcm_open(p_pb_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Cannot open audio device %s (%s)\n", PCM_DEVICE, snd_strerror(err));
		retcode = ALSA_ERROR;
	}

	if (retcode == SUCCESS)
		if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
			fprintf (stderr, "Cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
			retcode = ALSA_ERROR;
		}

	if (retcode == SUCCESS)
		if ((err = snd_pcm_hw_params_any (*p_pb_handle, hw_params)) < 0) {
			fprintf (stderr, "Cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
			retcode = ALSA_ERROR;
		}

	if (retcode == SUCCESS)
		if ((err = snd_pcm_hw_params_set_access (*p_pb_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			fprintf (stderr, "Cannot set access type (%s)\n", snd_strerror (err));
			retcode = ALSA_ERROR;
		}

	if (retcode == SUCCESS)
		if ((err = snd_pcm_hw_params_set_format (*p_pb_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
			fprintf (stderr, "Cannot set sample format (%s)\n", snd_strerror (err));
			retcode = ALSA_ERROR;
		}

	if (retcode == SUCCESS) {
		if ((err = snd_pcm_hw_params_set_rate_near (*p_pb_handle, hw_params, &tmp_srate, 0)) < 0) {
			fprintf (stderr, "Cannot set sample rate (%s)\n", snd_strerror (err));
			retcode = ALSA_ERROR;
		}
	}

	if (retcode == SUCCESS) {
		*p_sample_rate = (long)tmp_srate;
		if ((err = snd_pcm_hw_params_set_channels (*p_pb_handle, hw_params, 2)) < 0) {
			fprintf (stderr, "Cannot set channel count (%s)\n", snd_strerror (err));
			retcode = ALSA_ERROR;
		}
	}

	if (retcode == SUCCESS) {
		if ((err = snd_pcm_hw_params (*p_pb_handle, hw_params)) < 0) {
			fprintf (stderr, "Cannot set parameters (%s)\n", snd_strerror (err));
			retcode = ALSA_ERROR;
		}
	}

	if (retcode == SUCCESS) {
		snd_pcm_hw_params_free (hw_params);

		if ((err = snd_pcm_prepare (*p_pb_handle)) < 0) {
			fprintf (stderr, "Cannot prepare audio interface for use (%s)\n", snd_strerror (err));
			retcode = ALSA_ERROR;
		}
	}

	return (retcode);
}
#endif
