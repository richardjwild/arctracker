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

#include <signal.h>
#include "arctracker.h"
#include "config.h"
#include "read_mod.h"
#include "play_mod.h"
#include "oss.h"
#include "alsa.h"
#include "audio_api.h"
#include "configuration.h"

int main(int argc, char *argv[])
{
	return_status retcode = SUCCESS;
	args_t args;
	void *modfile = NULL;
	long modsize = 0;
	long sample_rate = DEFAULT_SAMPLERATE;
	int audio_fd;
	module_t module;
	sample_t samples[NUM_SAMPLES];

	short audio_buf[1024];
	int err;
	audio_api_t audio_api;

	read_configuration(argc, argv);
	retcode = get_arguments(
		argc,
		argv,
		&args);

	if (retcode == SUCCESS)
		retcode = load_file(
			args.mod_filename,
			&modfile,
			&modsize);

	if (retcode == SUCCESS)
		retcode = read_file(
			modfile,
			modsize,
			&module,
			samples);

	if (retcode == SUCCESS)
	{
		if (args.api == OSS)
			audio_api = initialise_oss(sample_rate, AUDIO_BUFFER_SIZE_FRAMES);
		else if (args.api == ALSA)
            audio_api = initialise_alsa(sample_rate, AUDIO_BUFFER_SIZE_FRAMES);
	}

	if (retcode == SUCCESS) {
		play_module(
				&module,
				samples,
				audio_api,
				&args);
	}

	audio_api.finish();

	if (modfile != NULL)
		free(modfile);
}
