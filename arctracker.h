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

#define EVENT_SIZE_SINGLE_EFFECT 4;
#define EVENT_SIZE_MULTIPLE_EFFECT 8;

typedef enum {
    NO_EFFECT,
    ARPEGGIO,
    PORTAMENTO_UP,
	PORTAMENTO_DOWN,
	BREAK_PATTERN,
	SET_TRACK_STEREO,
	VOLUME_SLIDE_UP,
	VOLUME_SLIDE_DOWN,
	JUMP_TO_POSITION,
	SET_TEMPO,
	SET_VOLUME_TRACKER,
	TONE_PORTAMENTO,
	VOLUME_SLIDE,
	SET_VOLUME_DESKTOP_TRACKER,
	PORTAMENTO_FINE,
	SET_TEMPO_FINE,
	VOLUME_SLIDE_FINE
} command_t;

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
	__uint8_t code;
	command_t command;
	__uint8_t data;
} effect_t;

typedef struct {
    __uint8_t note;
    __uint8_t sample;
    effect_t effects[4];
} channel_event_t;

typedef struct {
	char *format;
	char tracker_version[LEN_TRACKER_VERSION+1];
	char name[MAX_LEN_TUNENAME_DSKT+1];
	char author[MAX_LEN_AUTHOR_DSKT+1];
	long num_channels;
	unsigned char default_channel_stereo[MAX_CHANNELS_DSKT];
	long initial_speed;
	long tune_length;
	long num_patterns;
	long num_samples;
	sample_t *samples;
	unsigned char pattern_length[NUM_PATTERNS];
	unsigned char sequence[MAX_TUNELENGTH];
	void *patterns[NUM_PATTERNS];
    size_t (*decode_event)(const __uint32_t *raw, channel_event_t *decoded);
} module_t;

typedef struct {
	int position_in_sequence;
	int position_in_pattern;
	void *pattern_line_ptr;
	int counter;
	int speed;
	long sps_per_tick;
} positions_t;

#endif // ARCTRACKER_H