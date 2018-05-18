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
#include "configuration.h"
#include "read_mod.h"
#include "play_mod.h"
#include "oss.h"
#include "alsa.h"

int main(int argc, char *argv[])
{
	return_status retcode;
	void *modfile;
	long modsize;
	module_t module;
	sample_t samples[NUM_SAMPLES];
	audio_api_t audio_api;

	read_configuration(argc, argv);

	retcode = load_file(&modfile, &modsize);

	if (retcode == SUCCESS)
		retcode = read_file(
			modfile,
			modsize,
			&module,
			samples);

	if (retcode == SUCCESS)
	{
		switch (configuration().api)
		{
			case OSS:
				audio_api = initialise_oss();
				break;
			case ALSA:
				audio_api = initialise_alsa();
				break;
		}
	}

	if (retcode == SUCCESS) {
		play_module(&module, samples, audio_api);
	}

	audio_api.finish();

	if (modfile != NULL)
		free(modfile);
}
