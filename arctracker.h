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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include "config.h"

#ifdef HAVE_LIBARTSC
#include <artsc.h>
#endif

#ifdef HAVE_LIBASOUND
#include <alsa/asoundlib.h>
#endif

/* macro definitions */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

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
#define CHUNK_HEADER_SIZE 8
#define MAX_CHANNELS 8
#define MAX_CHANNELS_DSKT 16
#define NUM_PATTERNS 256
#define MAX_TUNELENGTH 128
#define NUM_SAMPLES 256
#define MAX_PATTERNLENGTH 64
#define MAX_VOLUME 255
#define MAX_NOTES 36
#define DEFAULT_SAMPLERATE 44100
#define DEVICE_NAME "/dev/dsp"
#define PCM_DEVICE "plughw:0,0"
#define BUF_SIZE 4096

#define ARG_PIANOLA "--pianola"
#define ARG_OSS "--oss"
#define ARG_ALSA "--alsa"
#define ARG_ARTS "--arts"
#define ARG_VOLUME "--volume="
#define ARG_LOOP "--loop"

#define DSKT_CHUNK "DskT"
#define ESKT_CHUNK "EskT"
#define DSKS_CHUNK "DskS"
#define ESKS_CHUNK "EskS"
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
	CANNOT_SET_SAMPLE_FORMAT,
	BAD_SAMPLE_FORMAT,
	CANNOT_SET_CHANNELS,
	BAD_CHANNELS,
	CANNOT_SET_SAMPLE_RATE,
	AUDIO_WRITE_ERROR,
	SAMPLE_INVALID,
	ARTS_ERROR,
	ALSA_ERROR,
	API_NOT_AVAILABLE
};
typedef enum return_status return_status;

enum yn {YES, NO};
typedef enum yn yn;

enum mono_stereo {MONO, STEREO};
typedef enum mono_stereo mono_stereo;

enum format {
	BITS_8_UNSIGNED,
	BITS_8_SIGNED,
	BITS_16_SIGNED_LITTLE_ENDIAN,
	BITS_16_SIGNED_BIG_ENDIAN,
	BITS_16_SIGNED_NATIVE_ENDIAN,
	BITS_16_UNSIGNED_LITTLE_ENDIAN,
	BITS_16_UNSIGNED_BIG_ENDIAN
};
typedef enum format format;

enum module_type {TRACKER, DESKTOP_TRACKER};
typedef enum module_type module_type;

enum output_api {NOT_SPECIFIED, OSS, ALSA, ARTS};
typedef enum output_api output_api;

typedef struct {
	char *mod_filename;
	int volume;
	yn pianola;
	output_api api;
	yn loop_forever;
} program_arguments;

typedef struct {
	module_type format;
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
} mod_details;

typedef struct {
	char name[MAX_LEN_SAMPLENAME_DSKT+1];
	long volume;
	long sample_length;
	long repeat_offset;
	long repeat_length;
	long note;
	long period;
	long sustain_start;
	long sustain_length;
	void *sample_data;
} sample_details;

typedef struct {
	int position_in_sequence;
	int position_in_pattern;
	void *pattern_line_ptr;
	int counter;
	int speed;
	long sps_per_tick;
} tune_info;

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
} current_event;

typedef struct {
	unsigned long phase_accumulator;
	unsigned long phase_acc_fraction;
	unsigned long phase_increment;
	int period;
	int target_period;
	yn sample_repeats;
	long repeat_offset;
	long sample_length;
	long repeat_length;
	void *sample_pointer;
	unsigned char volume;
	yn channel_currently_playing;
	long left_channel_multiplier;
	long right_channel_multiplier;
	unsigned char arpeggio_counter;
	unsigned char last_data_byte;
	unsigned char note_currently_playing;
} channel_info;


/* function prototypes */
return_status get_arguments(
	int p_argc,
	char *p_argv[],
	program_arguments *p_args);

return_status load_file(
	char *p_filename,
	void **p_array_ptr,
	long *p_bytes_loaded);

#ifdef HAVE_LIBARTSC
return_status initialise_arts(
	arts_stream_t *p_stream,
	long p_sample_rate,
	mono_stereo *p_stereo_mode,
	format *p_sample_format);
#endif

#ifdef HAVE_LIBASOUND
return_status initialise_alsa(
	snd_pcm_t **p_pb_handle,
	long *p_sample_rate,
	mono_stereo *p_stereo_mode,
	format *p_sample_format);
#endif

return_status initialise_oss(
	int *p_audio_fd,
	long *p_sample_rate,
	mono_stereo *p_stereo_mode,
	format *p_sample_format);

return_status read_file(
	void *p_modfile,
	long p_modsize,
	mod_details *p_module,
	sample_details *p_samples);

return_status read_tracker_file(
	void *p_modfile,
	long p_modsize,
	mod_details *p_module,
	sample_details *p_samples);

return_status read_desktop_tracker_file(
	void *p_modfile,
	long p_modsize,
	mod_details *p_module,
	sample_details *p_samples);

return_status search_tff(
	void *p_searchfrom,
	void **p_chunk_address,
	long p_array_end,
	char *p_chunk,
	long p_occurrence);

void read_nchar(
	char *p_output,
	void *p_input,
	int p_num_chars,
	yn p_null_term);

void read_nbytes(
	long *p_output,
	void *p_input,
	int p_num_bytes);

return_status get_patterns(
	void *p_search_from,
	long p_array_end,
	void **p_patterns,
	int *p_num_patterns);

return_status get_samples(
	void *p_search_from,
	long p_array_end,
	int *p_samples_found,
	sample_details *p_samples);

return_status get_sample_info(
	void *p_search_from,
	long p_array_end,
	sample_details *p_sample,
	int p_sample_number);

return_status validate_modfile(
	mod_details *p_module,
	sample_details *p_samples,
	int p_num_samples);

return_status validate_modfile(
	mod_details *p_module,
	sample_details *p_sample,
	int p_num_samples);

return_status initialise_phase_incrementor_values(
	long **p_phase_incrementors,
	unsigned int *p_periods,
	long p_sample_rate);

return_status play_module(
	mod_details *p_module,
	sample_details *p_sample,
	void *p_ah_ptr,
	long p_sample_rate,
	long *p_phase_incrementors,
	unsigned int *p_periods,
	format p_sample_format,
	mono_stereo p_stereo_mode,
	program_arguments *p_args);

__inline void initialise_values(
	mono_stereo p_stereo_mode,
	format p_sample_format,
	char *buffer_shifter,
	tune_info *p_current_positions,
	channel_info *p_voice_info,
	mod_details *p_module,
	yn p_pianola,
	long p_sample_rate);

__inline yn update_counters(
	tune_info *p_current_positions,
	mod_details *p_module,
	yn p_pianola);

__inline void get_current_pattern_line(
	tune_info *p_current_positions,
	mod_details *p_module,
	current_event *p_current_pattern_line,
	yn p_pianola);

__inline void get_new_note(
	current_event *p_current_event,
	sample_details *p_sample,
	channel_info *p_current_voice,
	long *p_phase_incrementors,
	unsigned int *p_periods,
	yn p_tone_portamento,
	module_type p_module_type,
	long p_num_samples);

__inline void process_tracker_command(
	current_event *p_current_event,
	channel_info *p_current_voice,
	tune_info *p_current_positions,
	long *p_phase_incrementors,
	mod_details *p_module,
	unsigned int *p_periods,
	yn on_event);

__inline void process_desktop_tracker_command(
	current_event *p_current_event,
	channel_info *p_current_voice,
	tune_info *p_current_positions,
	long *p_phase_incrementors,
	mod_details *p_module,
	unsigned int *p_periods,
	yn on_event,
	long p_sample_rate);

__inline void prepare_current_frame_sample_data(
	long *p_current_frame_left_channel,
	long *p_current_frame_right_channel,
	channel_info *p_voice_info,
	mod_details *p_module,
	mono_stereo p_stereo_mode,
	unsigned char p_volume);

__inline void mix_channels(
	long *p_current_frame_left_channel,
	long *p_current_frame_right_channel,
	channel_info *p_voice_info,
	mod_details *p_module,
	long *p_left_channel,
	long *p_right_channel);

/*
__inline return_status write_audio_data (
	output_api p_api,
	format p_sample_format,
	mono_stereo p_stereo_mode,
	long p_left_channel,
	long p_right_channel,
	char p_buffer_shifter,
	void *p_ah_ptr)
*/

__inline return_status write_audio_data(
	output_api p_api,
	channel_info *p_voice_info,
	mod_details *p_module,
	mono_stereo p_stereo_mode,
	unsigned char p_volume,
	format p_sample_format,
	char p_buffer_shifter,
	void *p_ah_ptr,
	long p_nframes);

__inline void write_channel_audio_data(
	int p_ch,
	channel_info *p_voice_info,
	long p_nframes,
	char p_buffer_shifter,
	long p_bufptr,
	mono_stereo p_stereo_mode,
	unsigned char p_volume,
	int stridelen);

__inline return_status output_data(
	output_api p_api,
	char p_buffer_shifter,
	format p_sample_format,
	mono_stereo p_stereo_mode,
	void *p_ah_ptr,
	long p_num_channels);
