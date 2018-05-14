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

#ifndef ARCTRACKER_H
#define ARCTRACKER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include "config.h"
#include "audio_api.h"

/* macro definitions */

#define READONLY "r"
#define STRINGS_MATCH 0

#define ARRAY_CHUNK_SIZE 16384

#define LEN_TRACKER_VERSION 4
#define MAX_LEN_TUNENAME 32
#define MAX_LEN_TUNENAME_DSKT 64
#define MAX_LEN_AUTHOR 32
#define MAX_LEN_AUTHOR_DSKT 64
#define MAX_LEN_SAMPLENAME 20
#define MAX_LEN_SAMPLENAME_DSKT 32

#define CHUNKSIZE 4
#define MAX_CHANNELS 16
#define MAX_CHANNELS_DSKT 16
#define NUM_PATTERNS 256
#define MAX_TUNELENGTH 128
#define NUM_SAMPLES 256
#define DEFAULT_SAMPLERATE 44100
#define AUDIO_BUFFER_SIZE_FRAMES 1024

#define ARG_PIANOLA "--pianola"
#define ARG_OSS "--oss"
#define ARG_ALSA "--alsa"
#define ARG_VOLUME "--volume="
#define ARG_LOOP "--loop"

#define DSKT_CHUNK "DskT"
#define MUSX_CHUNK "MUSX"
#define TINF_CHUNK "TINF"
#define MVOX_CHUNK "MVOX"
#define STER_CHUNK "STER"
#define MNAM_CHUNK "MNAM"
#define ANAM_CHUNK "ANAM"
#define MLEN_CHUNK "MLEN"
#define PNUM_CHUNK "PNUM"
#define PLEN_CHUNK "PLEN"
#define SEQU_CHUNK "SEQU"
#define PATT_CHUNK "PATT"
#define SAMP_CHUNK "SAMP"
#define SNAM_CHUNK "SNAM"
#define SVOL_CHUNK "SVOL"
#define SLEN_CHUNK "SLEN"
#define ROFS_CHUNK "ROFS"
#define RLEN_CHUNK "RLEN"
#define SDAT_CHUNK "SDAT"

#define ARPEGGIO_COMMAND 0 /* 0 */
#define PORTUP_COMMAND 1 /* 1 */
#define PORTDOWN_COMMAND 2 /* 2 */
#define BREAK_COMMAND 11 /* B */
#define STEREO_COMMAND 14 /* E */
#define VOLSLIDEUP_COMMAND 16 /* G */
#define VOLSLIDEDOWN_COMMAND 17 /* H */
#define JUMP_COMMAND 19 /* J */
#define SPEED_COMMAND 28 /* S */
#define VOLUME_COMMAND 31 /* V */
#define ARPEGGIO_COMMAND_DSKT 0x0
#define PORTUP_COMMAND_DSKT 0x1
#define PORTDOWN_COMMAND_DSKT 0x2
#define TONEPORT_COMMAND_DSKT 0x3
#define VIBRATO_COMMAND_DSKT 0x4 /* not implemented yet */
#define DELAYEDNOTE_COMMAND_DSKT 0x5 /* not implemented yet */
#define RELEASESAMP_COMMAND_DSKT 0x6 /* not implemented yet */
#define TREMOLO_COMMAND_DSKT 0x7 /* not implemented yet */
#define PHASOR_COMMAND1_DSKT 0x8 /* not implemented yet */
#define PHASOR_COMMAND2_DSKT 0x9 /* not implemented yet */
#define VOLSLIDE_COMMAND_DSKT 0xa
#define JUMP_COMMAND_DSKT 0xb
#define VOLUME_COMMAND_DSKT 0xc
#define STEREO_COMMAND_DSKT 0xd
#define STEREOSLIDE_COMMAND_DSKT 0xe /* not implemented yet */
#define SPEED_COMMAND_DSKT 0xf
#define ARPEGGIOSPEED_COMMAND_DSKT 0x10 /* not implemented yet */
#define FINEPORTAMENTO_COMMAND_DSKT 0x11
#define CLEAREPEAT_COMMAND_DSKT 0x12 /* not implemented yet */
#define SETVIBRATOWAVEFORM_COMMAND_DSKT 0x14 /* not implemented yet */
#define LOOP_COMMAND_DSKT 0x16 /* not implemented yet */
#define SETTREMOLOWAVEFORM_COMMAND_DSKT 0x17 /* not implemented yet */
#define SETFINETEMPO_COMMAND_DSKT 0x18
#define RETRIGGERSAMPLE_COMMAND_DSKT 0x19 /* not implemented yet */
#define FINEVOLSLIDE_COMMAND_DSKT 0x1a
#define HOLD_COMMAND_DSKT 0x1b /* not implemented yet */
#define NOTECUT_COMMAND_DSKT 0x1c /* not implemented yet */
#define NOTEDELAY_COMMAND_DSKT 0x1d /* not implemented yet */
#define PATTERNDELAY_COMMAND_DSKT 0x1e /* not implemented yet */

/*#define DEVELOPING*/

/* type definitions */
enum return_status {
	SUCCESS,
	BAD_ARGUMENTS,
	BAD_FILE,
	MEMORY_FAILURE,
	CHUNK_NOT_FOUND,
	NOT_MODULE,
	FILE_CORRUPT,
	NO_PATTERNS_IN_MODULE,
	NO_SAMPLES_IN_MODULE,
	CANNOT_OPEN_AUDIO_DEVICE,
	CANNOT_SET_SAMPLE_RATE,
	AUDIO_WRITE_ERROR,
	SAMPLE_INVALID,
	ALSA_ERROR,
	API_NOT_AVAILABLE
};
typedef enum return_status return_status;

enum module_type {TRACKER, DESKTOP_TRACKER};
typedef enum module_type module_type_t;

enum output_api {NOT_SPECIFIED, OSS, ALSA};
typedef enum output_api output_api;

typedef struct {
	char *mod_filename;
	int volume;
	bool pianola;
	output_api api;
	bool loop_forever;
} args_t;

typedef struct {
	module_type_t format;
	char tracker_version[LEN_TRACKER_VERSION+1];
	char name[MAX_LEN_TUNENAME_DSKT+1];
	char author[MAX_LEN_AUTHOR_DSKT+1];
	long num_channels;
	unsigned char default_channel_stereo[MAX_CHANNELS_DSKT];
	long initial_speed;
	long tune_length;
	long num_patterns;
	long num_samples;
	unsigned char pattern_length[NUM_PATTERNS];
	unsigned char sequence[MAX_TUNELENGTH];
	void *patterns[NUM_PATTERNS];
} module_t;

typedef struct {
	char name[MAX_LEN_SAMPLENAME_DSKT+1];
	long default_gain;
	long sample_length;
	bool repeats;
	long repeat_offset;
	long repeat_length;
	unsigned char transpose;
	long period;
	long sustain_start;
	long sustain_length;
	void *sample_data;
} sample_t;

typedef struct {
	int position_in_sequence;
	int position_in_pattern;
	void *pattern_line_ptr;
	int counter;
	int speed;
	long sps_per_tick;
} positions_t;

typedef struct {
	unsigned char note;
	unsigned char sample;
	unsigned char command;
	unsigned char data;
	unsigned char command1;
	unsigned char data1;
	unsigned char command2;
	unsigned char data2;
	unsigned char command3;
	unsigned char data3;
} channel_event_t;

/* function prototypes */
return_status get_arguments(
	int p_argc,
	char *p_argv[],
	args_t *p_args);

return_status load_file(
	char *p_filename,
	void **p_array_ptr,
	long *p_bytes_loaded);

#endif // ARCTRACKER_H