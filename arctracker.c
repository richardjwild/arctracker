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

	/* Sample periods - one for each of the 36 notes.  The reason why we don't store   *
	 * phase increment values directly is because the portamento up and down commands  *
	 * operate on this period value, which is used to obtain the phase increment later *
	 * by looking up from a table.  We implement it the same way to ensure maximum     *
	 * fidelity to the original tracker playback.                                      */

	unsigned int periods[] =
		{0x06A0,0x0650,0x05F4,0x05A0,
		0x054C,0x0500,0x04B8,0x0474,
		0x0434,0x03F8,0x03C0,0x038A,

		0x0358,0x0328,0x02FA,0x02D0,
		0x02A6,0x0280,0x025C,0x023A,
		0x021A,0x01FC,0x01E0,0x01C5,
		0x01AC,0x0194,0x017D,0x0168,
		0x0153,0x0140,0x012E,0x011D,
		0x010D,0x00FE,0x00F0,0x00E2,
		0x00D6,0x00CA,0x00BE,0x00B4,
		0x00AA,0x00A0,0x0097,0x008F,
		0x0087,0x007F,0x0078,0x0071,
		0x006B,
		0x0065,0x005E,0x005A,
		0x0055,0x0050,0x004C,0x0048,
		0x0044,0x0040,0x003C,0x0039,
		0x0036};

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
				periods,
				&args);
	}

	audio_api.finish();

	if (modfile != NULL)
		free(modfile);
}
