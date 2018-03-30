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
#include "log_lin_tab.h"

char *notes[] = {"---",
	"C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
	"C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
	"C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3"};

char alphanum[] = {'-',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z'};

long left_channel_multiplier[] = {256, 212, 172, 128, 84, 44, 0};
long right_channel_multiplier[] = {0, 44, 84, 128, 172, 212, 256};

unsigned char audio_buffer[BUF_SIZE];
long channel_buffer[BUF_SIZE * MAX_CHANNELS];

return_status play_module(
	mod_details *p_module,
	sample_details *p_sample,
	void *p_ah_ptr,
	long p_sample_rate,
	long *p_phase_incrementors,
	unsigned int *p_periods,
	format p_sample_format,
	mono_stereo p_stereo_mode,
	program_arguments *p_args)
{
	return_status retcode = SUCCESS;
	int channel;
	long current_frame_left_channel[MAX_CHANNELS];
	long current_frame_right_channel[MAX_CHANNELS];
	long left_channel;
	long right_channel;

	tune_info current_positions;
	current_event current_pattern_line[MAX_CHANNELS];
	current_event *current_pattern_line_ptr;
	channel_info voice_info[MAX_CHANNELS];
	channel_info *voice_info_ptr;

	sample_details *sample_info_ptr;

	char buffer_shifter;

	yn looped_yet = NO;

	initialise_values(
		p_stereo_mode,
		p_sample_format,
		&buffer_shifter,
		&current_positions,
		voice_info,
		p_module,
		p_args->pianola,
		p_sample_rate);

	/* loop through whole tune */
	do {
		if (++(current_positions.counter) == current_positions.speed) {
			/* new event. update counters: current position in pattern, position in sequence */
			looped_yet = update_counters(
				&current_positions,
				p_module,
				p_args->pianola);

			/* we have a new pattern line to process */
			get_current_pattern_line(
				&current_positions,
				p_module,
				current_pattern_line,
				p_args->pianola);

			/* current pattern line now held in current_pattern_line */

			for (channel = 0, current_pattern_line_ptr=current_pattern_line, voice_info_ptr=voice_info;
			channel < p_module->num_channels;
			channel++, current_pattern_line_ptr++, voice_info_ptr++) {
				if (current_pattern_line_ptr->note) {
					/* new note to play - get sample details */
					if (current_pattern_line_ptr->command != TONEPORT_COMMAND_DSKT) {
						get_new_note(
							current_pattern_line_ptr,
							p_sample,
							voice_info_ptr,
							p_phase_incrementors,
							p_periods,
							NO,
							p_module->format,
							p_module->num_samples);
					} else {
						get_new_note(
							current_pattern_line_ptr,
							p_sample,
							voice_info_ptr,
							p_phase_incrementors,
							p_periods,
							YES,
							p_module->format,
							p_module->num_samples);
					}
				} else if (current_pattern_line_ptr->sample) {
					/* this was a strange feature of tracker (and soundtracker) - I never knew *
					 * whether it was meant or an accident.  If a sample number is specified   *
					 * although no note is present, it resets the volume back to the sample's  *
					 * default.  The phase accumulator is left alone, i.e. the sample is not   *
					 * retriggered.  This was useful, as it was effectively a "volume" command *
					 * for free: the command field is free for another effect to be used. I    *
					 * have used this effect in a few modfiles, so I am implementing the same  *
					 * behaviour here.                                                         */

					sample_info_ptr = p_sample;
					sample_info_ptr += (current_pattern_line_ptr->sample - 1);

					if (p_module->format == TRACKER)
						voice_info_ptr->volume = sample_info_ptr->volume;
					else
						voice_info_ptr->volume = ((sample_info_ptr->volume + 1) << 1) - 1;
				}

				/* new command to execute */
				if (p_module->format == TRACKER)
					process_tracker_command(
						current_pattern_line_ptr,
						voice_info_ptr,
						&current_positions,
						p_phase_incrementors,
						p_module,
						p_periods,
						YES);
				else
					process_desktop_tracker_command(
						current_pattern_line_ptr,
						voice_info_ptr,
						&current_positions,
						p_phase_incrementors,
						p_module,
						p_periods,
						YES,
						p_sample_rate);
			}
		} else {
			/* no note as this is between events, but we may have some commands to process */
			for (channel = 0, current_pattern_line_ptr=current_pattern_line, voice_info_ptr=voice_info;
			channel < p_module->num_channels;
			channel++, current_pattern_line_ptr++, voice_info_ptr++) {
				/* new command to execute */
				if (p_module->format == TRACKER)
					process_tracker_command(
						current_pattern_line_ptr,
						voice_info_ptr,
						&current_positions,
						p_phase_incrementors,
						p_module,
						p_periods,
						NO);
				else
					process_desktop_tracker_command(
						current_pattern_line_ptr,
						voice_info_ptr,
						&current_positions,
						p_phase_incrementors,
						p_module,
						p_periods,
						NO,
						p_sample_rate);
			}
		}

		/* write one tick's worth of audio data */
		retcode = write_audio_data(
			p_args->api,
			voice_info,
			p_module,
			p_stereo_mode,
			p_args->volume,
			p_sample_format,
			buffer_shifter,
			p_ah_ptr,
			current_positions.sps_per_tick);
	}
	while (((looped_yet == NO) || (p_args->loop_forever == YES)) && (retcode == SUCCESS));

	if (p_args->pianola == NO)
		printf("\n");

	return (retcode);
}

/* initialise_values function.                    *
 * Set up values in preparation for player start. */

__inline void initialise_values(
	mono_stereo p_stereo_mode,
	format p_sample_format,
	char *buffer_shifter,
	tune_info *p_current_positions,
	channel_info *p_voice_info,
	mod_details *p_module,
	yn p_pianola,
	long p_sample_rate)
{
	int channel;

	if (p_stereo_mode == STEREO) {
		if ((p_sample_format == BITS_8_SIGNED) || (p_sample_format == BITS_8_UNSIGNED))
			*buffer_shifter = 1;
		else
			*buffer_shifter = 2;
	} else {
		if ((p_sample_format == BITS_8_SIGNED) || (p_sample_format == BITS_8_UNSIGNED))
			*buffer_shifter = 0;
		else
			*buffer_shifter = 1;
	}

	p_current_positions->position_in_sequence = 0;
	p_current_positions->position_in_pattern = -1;
	p_current_positions->counter = p_module->initial_speed - 1;
	p_current_positions->speed = p_module->initial_speed;
	p_current_positions->pattern_line_ptr = p_module->patterns[p_module->sequence[0]];

#ifdef DEVELOPING
	if (p_pianola == YES)
		printf("Pianola mode on\n");
	else if (p_pianola == NO)
		printf("Pianola mode off\n");
#endif

	/* initialise voice info: all voices silent and set initial stereo positions */
	for (channel = 0; channel < p_module->num_channels; channel++) {
		p_voice_info[channel].channel_currently_playing = NO;
		p_voice_info[channel].left_channel_multiplier =
		left_channel_multiplier[p_module->default_channel_stereo[channel] - 1];
		p_voice_info[channel].right_channel_multiplier =
		right_channel_multiplier[p_module->default_channel_stereo[channel] - 1];
	}

	if (p_pianola == NO) {
		printf("Playing position 1 of %d", p_module->tune_length);
		fflush(stdout);	
	}

	p_current_positions->sps_per_tick = (p_sample_rate << 8)/50;
}

/* update_counters function.                                         *
 * Called every n tracker periods where n is the current tune speed; *
 * updates position in pattern and position in sequence counters.    */

__inline yn update_counters(
	tune_info *p_current_positions,
	mod_details *p_module,
	yn p_pianola)
{
	unsigned char *sequence_ptr;
	void **patterns_list_ptr;
	yn looped_yet = NO;
	
	p_current_positions->counter = 0;
	if (++(p_current_positions->position_in_pattern) ==
	p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]]) {
		if (++p_current_positions->position_in_sequence == p_module->tune_length) {
			p_current_positions->position_in_sequence = 0;
			looped_yet = YES;
		}
		p_current_positions->position_in_pattern = 0;

		/* get address of pattern */
		patterns_list_ptr = p_module->patterns;
		sequence_ptr = p_module->sequence;
		sequence_ptr += p_current_positions->position_in_sequence;
		patterns_list_ptr += *sequence_ptr;
		p_current_positions->pattern_line_ptr = *patterns_list_ptr;

		if (p_pianola == NO) {
			printf(
				"%cPlaying position %d of %d ",
				13,
				p_current_positions->position_in_sequence + 1,
				p_module->tune_length);
			fflush(stdout);
		}
	}

	return (looped_yet);
}

/* get_current_pattern_line function.                                *
 * Called every n tracker periods where n is the current tune speed; *
 * gets current pattern line from pattern data and reads data into a *
 * structure: note, sample, command, command data.                   */

__inline void get_current_pattern_line(
	tune_info *p_current_positions,
	mod_details *p_module,
	current_event *p_current_pattern_line,
	yn p_pianola)
{
	int channel;
	unsigned int tmp;
	unsigned char *sequence_ptr;
	void *pattern_line_ptr;
	void **patterns_list_ptr;
	current_event *current_pattern_line_ptr;

	if (p_pianola == YES)
		printf(
			"%2d %2d | ",
			p_current_positions->position_in_sequence,
			p_current_positions->position_in_pattern);

	pattern_line_ptr = p_current_positions->pattern_line_ptr;

	for (channel = 0, current_pattern_line_ptr=p_current_pattern_line;
	channel < p_module->num_channels;
	channel++, current_pattern_line_ptr++) {
		if (p_module->format == TRACKER) {
			current_pattern_line_ptr->data = *(char *)pattern_line_ptr++;
			current_pattern_line_ptr->command = *(char *)pattern_line_ptr++;
			current_pattern_line_ptr->sample = *(char *)pattern_line_ptr++;
			current_pattern_line_ptr->note = *(char *)pattern_line_ptr++;
		} else /* if (p_module->format == desktop_tracker) */ {
#ifdef WORDS_BIGENDIAN
			tmp = *(unsigned int *)pattern_line_ptr;
			tmp = ((tmp & 0xff) << 24) | ((tmp & 0xff00) << 8) |
			((tmp & 0xff0000) >> 8) | ((tmp & 0xff000000) >> 24);

			current_pattern_line_ptr->note = (tmp & 0xfc0) >> 6;
			current_pattern_line_ptr->sample = tmp & 0x3f;

			if (tmp & (0x1f << 17)) {
				/* four commands */
				current_pattern_line_ptr->command = (tmp & 0x1f000) >> 12;
				current_pattern_line_ptr->command1 = (tmp & 0x3e0000) >> 17;
				current_pattern_line_ptr->command2 = (tmp & 0x7c00000) >> 22;
				current_pattern_line_ptr->command3 = (tmp & 0xf8000000) >> 27;
				pattern_line_ptr += 4;
				tmp = *(unsigned int *)pattern_line_ptr;
				tmp = ((tmp & 0xff) << 24) | ((tmp & 0xff00) << 8) |
				((tmp & 0xff0000) >> 8) | ((tmp & 0xff000000) >> 24);
				current_pattern_line_ptr->data = tmp & 0xff;
				current_pattern_line_ptr->data1 = (tmp & 0xff00) >> 8;
				current_pattern_line_ptr->data2 = (tmp & 0xff0000) >> 16;
				current_pattern_line_ptr->data3 = (tmp & 0xff000000) >> 24;
				pattern_line_ptr += 4;
			} else {
				/* one command */
				current_pattern_line_ptr->data = (tmp & 0xff000000) >> 24;
				current_pattern_line_ptr->data1 = 0;
				current_pattern_line_ptr->data2 = 0;
				current_pattern_line_ptr->data3 = 0;
				current_pattern_line_ptr->command = (tmp & 0x1f000) >> 12;
				current_pattern_line_ptr->command1 = 0;
				current_pattern_line_ptr->command2 = 0;
				current_pattern_line_ptr->command3 = 0;
				pattern_line_ptr += 4;
			}
#else
			current_pattern_line_ptr->note = (*(unsigned int *)pattern_line_ptr & 0xfc0) >> 6;
			current_pattern_line_ptr->sample = *(unsigned int *)pattern_line_ptr & 0x3f;

			if (*(unsigned int *)pattern_line_ptr & (0x1f << 17)) {
				/* four commands */
				current_pattern_line_ptr->command = (*(unsigned int *)pattern_line_ptr & 0x1f000) >> 12;
				current_pattern_line_ptr->command1 = (*(unsigned int *)pattern_line_ptr & 0x3e0000) >> 17;
				current_pattern_line_ptr->command2 = (*(unsigned int *)pattern_line_ptr & 0x7c00000) >> 22;
				current_pattern_line_ptr->command3 = (*(unsigned int *)pattern_line_ptr & 0xf8000000) >> 27;
				pattern_line_ptr += 4;
				current_pattern_line_ptr->data = *(unsigned int *)pattern_line_ptr & 0xff;
				current_pattern_line_ptr->data1 = (*(unsigned int *)pattern_line_ptr & 0xff00) >> 8;
				current_pattern_line_ptr->data2 = (*(unsigned int *)pattern_line_ptr & 0xff0000) >> 16;
				current_pattern_line_ptr->data3 = (*(unsigned int *)pattern_line_ptr & 0xff000000) >> 24;
				pattern_line_ptr += 4;
			} else {
				/* one command */
				current_pattern_line_ptr->data = (*(unsigned int *)pattern_line_ptr & 0xff000000) >> 24;
				current_pattern_line_ptr->data1 = 0;
				current_pattern_line_ptr->data2 = 0;
				current_pattern_line_ptr->data3 = 0;
				current_pattern_line_ptr->command = (*(unsigned int *)pattern_line_ptr & 0x1f000) >> 12;
				current_pattern_line_ptr->command1 = 0;
				current_pattern_line_ptr->command2 = 0;
				current_pattern_line_ptr->command3 = 0;
				pattern_line_ptr += 4;
			}
#endif
		}

		if (p_pianola == YES) {
			if (p_module->format == TRACKER)
				printf(
					"%s %c%c%X%X | ",
					notes[current_pattern_line_ptr->note],
					alphanum[current_pattern_line_ptr->sample],
					alphanum[current_pattern_line_ptr->command + 1],
					(current_pattern_line_ptr->data >> 4) & 0xf,
					current_pattern_line_ptr->data & 0xf);
			else
				printf(
					"%s %2X | ",
					notes[current_pattern_line_ptr->note],
					current_pattern_line_ptr->sample);
		}
	}
	if (p_pianola == YES)
		printf("\n");

	/* remember pattern line address for next event */
	p_current_positions->pattern_line_ptr = pattern_line_ptr;
}

/* get_new_note function.                                                             *
 * sets up voice to play a new note: sets sample pointer to the start of the relevant *
 * sample data, sets repeat offset and length if the sample is to repeat, sets phase  *
 * incrementor to the correct value depending on the note (pitch) to be played.       */

__inline void get_new_note(
	current_event *p_current_event,
	sample_details *p_sample,
	channel_info *p_current_voice,
	long *p_phase_incrementors,
	unsigned int *p_periods,
	yn p_tone_portamento,
	module_type p_module_type,
	long p_num_samples)
{
	unsigned int *periods_ptr;
	long *phase_incrementors_ptr;
	sample_details *sample_info_ptr;

	if (p_current_event->sample <= p_num_samples) {
		sample_info_ptr = p_sample;
		sample_info_ptr += (p_current_event->sample - 1);

		if (p_tone_portamento == NO || p_current_voice->channel_currently_playing == NO) {
			p_current_voice->channel_currently_playing = YES;
			p_current_voice->sample_pointer = sample_info_ptr->sample_data;
			p_current_voice->phase_accumulator = 0;
			p_current_voice->phase_acc_fraction = 0;
			p_current_voice->repeat_offset = sample_info_ptr->repeat_offset;
			p_current_voice->repeat_length = sample_info_ptr->repeat_length;
			p_current_voice->volume = sample_info_ptr->volume;
			p_current_voice->arpeggio_counter = 0;
			p_current_voice->note_currently_playing = p_current_event->note;
							
			if (p_module_type == TRACKER) {
				if (p_current_voice->repeat_length == 2) {
					/* tracker module, repeat length 2 means no repeat */
					p_current_voice->sample_repeats = NO;
					p_current_voice->sample_length = sample_info_ptr->sample_length;
				} else {
					p_current_voice->sample_repeats = YES;
					p_current_voice->sample_length = sample_info_ptr->repeat_offset + sample_info_ptr->repeat_length;
				}
			} else {
				if (p_current_voice->repeat_length) {
					/* desktop tracker module, repeat length 0 means no repeat */
					p_current_voice->sample_repeats = YES;
					p_current_voice->sample_length = sample_info_ptr->repeat_offset + sample_info_ptr->repeat_length;
				} else {
					p_current_voice->sample_repeats = NO;
					p_current_voice->sample_length = sample_info_ptr->sample_length;
				}
			}

			/* get period and phase incrementor value */
			phase_incrementors_ptr = p_phase_incrementors;
			periods_ptr = p_periods;

			if (p_module_type == TRACKER) {
				periods_ptr += (p_current_event->note + 12); /* desktop tracker has greater chromatic range */
			} else {
				p_current_voice->note_currently_playing += (13 - sample_info_ptr->note);
				periods_ptr += (p_current_event->note + 13 + (13 - sample_info_ptr->note));
				p_current_voice->volume = ((p_current_voice->volume + 1) << 1) - 1; /* desktop tracker volumes from 0..127 not 0..255 */
			}
			phase_incrementors_ptr += *periods_ptr;

			p_current_voice->period = *periods_ptr;
			p_current_voice->target_period = *periods_ptr;
			p_current_voice->phase_increment = *phase_incrementors_ptr;
		} else {
			/* there is a tone portamento command happening; do not play the note immediately *
			 * but set the pitch as a target and slide up (or down) towards it                */
			periods_ptr = p_periods;

			if (p_module_type == TRACKER) {
				periods_ptr += (p_current_event->note + 12); /* desktop tracker has greater chromatic range */
			} else {
				p_current_voice->note_currently_playing += (13 - sample_info_ptr->note);
				periods_ptr += (p_current_event->note + 13 + (13 - sample_info_ptr->note));
			}

			p_current_voice->target_period = *periods_ptr;
		}
	} else {
		p_current_voice->channel_currently_playing = NO;
	}
}

/* process_tracker_command function.  *
 * process a tracker command.         */

__inline void process_tracker_command(
	current_event *p_current_event,
	channel_info *p_current_voice,
	tune_info *p_current_positions,
	long *p_phase_incrementors,
	mod_details *p_module,
	unsigned int *p_periods,
	yn on_event)
{
	long *left_channel_multiplier_ptr;
	long *right_channel_multiplier_ptr;
	long *phase_incrementors_ptr;
	unsigned char temporary_note;
	unsigned int *periods_ptr;

	switch (p_current_event->command) {
	case VOLUME_COMMAND:
		if (on_event == YES)
			p_current_voice->volume = p_current_event->data;
		break;

	case SPEED_COMMAND:
		if (p_current_event->data /* ensure an "S00" command does not hang player! */
		    && (on_event == YES))
			p_current_positions->speed = p_current_event->data;
		break;

	case STEREO_COMMAND:
		if (on_event == YES) {
			left_channel_multiplier_ptr = left_channel_multiplier;
			right_channel_multiplier_ptr = right_channel_multiplier;
			left_channel_multiplier_ptr += (p_current_event->data - 1);
			right_channel_multiplier_ptr += (p_current_event->data - 1);
			p_current_voice->left_channel_multiplier = *left_channel_multiplier_ptr;
			p_current_voice->right_channel_multiplier = *right_channel_multiplier_ptr;
		}
		break;

	case VOLSLIDEUP_COMMAND:
		if ((255 - p_current_voice->volume) > p_current_event->data)
			p_current_voice->volume += p_current_event->data;
		else
			p_current_voice->volume = 255;
		break;

	case VOLSLIDEDOWN_COMMAND:
		if (p_current_voice->volume >= p_current_event->data)
			p_current_voice->volume -= p_current_event->data;
		else
			p_current_voice->volume = 0;
		break;

	case PORTUP_COMMAND:
		p_current_voice->period -= p_current_event->data;
		if (p_current_voice->period < 0x50)
			p_current_voice->period = 0x50;
		phase_incrementors_ptr = p_phase_incrementors;
		phase_incrementors_ptr += p_current_voice->period;
		p_current_voice->phase_increment = *phase_incrementors_ptr;
		break;

	case PORTDOWN_COMMAND:
		p_current_voice->period += p_current_event->data;
		if (p_current_voice->period > 0x3f0)
			p_current_voice->period = 0x3f0;
		phase_incrementors_ptr = p_phase_incrementors;
		phase_incrementors_ptr += p_current_voice->period;
		p_current_voice->phase_increment = *phase_incrementors_ptr;
		break;

	case TONEPORT_COMMAND_DSKT:
		if (p_current_event->data) {
			p_current_voice->last_data_byte = p_current_event->data;
		} else {
			p_current_event->data = p_current_voice->last_data_byte;
		}
		if (p_current_voice->period < p_current_voice->target_period) {
			p_current_voice->period += p_current_event->data;
			if (p_current_voice->period > p_current_voice->target_period) {
				p_current_voice->period = p_current_voice->target_period;
			}
		} else {
			p_current_voice->period -= p_current_event->data;
			if (p_current_voice->period < p_current_voice->target_period) {
				p_current_voice->period = p_current_voice->target_period;
			}
		}
		phase_incrementors_ptr = p_phase_incrementors;
		phase_incrementors_ptr += p_current_voice->period;
		p_current_voice->phase_increment = *phase_incrementors_ptr;
		break;

	case ARPEGGIO_COMMAND:
		if (p_current_event->data) {
			if (p_current_voice->arpeggio_counter == 0)
				temporary_note = p_current_voice->note_currently_playing;
			else if (p_current_voice->arpeggio_counter == 1) {
				temporary_note = p_current_voice->note_currently_playing +
					         ((p_current_event->data & 0xf0) >> 4);

				if (temporary_note > 36)
					temporary_note = p_current_voice->note_currently_playing;
			}
			else if (p_current_voice->arpeggio_counter == 2) {
				temporary_note = p_current_voice->note_currently_playing +
								(p_current_event->data & 0xf);

				if (temporary_note > 36)
					temporary_note = p_current_voice->note_currently_playing;
			}

			if (++(p_current_voice->arpeggio_counter) == 3)
				p_current_voice->arpeggio_counter = 0;

			phase_incrementors_ptr = p_phase_incrementors;
			periods_ptr = p_periods;

			periods_ptr += (temporary_note + 12);
			phase_incrementors_ptr += *periods_ptr;

			p_current_voice->period = *periods_ptr;
			p_current_voice->phase_increment = *phase_incrementors_ptr;
		}
		break;

	case BREAK_COMMAND:
		/* jog position (in pattern) to last event */
		if (on_event == YES)
			p_current_positions->position_in_pattern =
				p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]] - 1;
		break;

	case JUMP_COMMAND:
		/* move position to last event before the requested sequence position */
		if (on_event == YES) {
			p_current_positions->position_in_sequence = p_current_event->data - 1;
			p_current_positions->position_in_pattern =
				p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]] - 1;
		}
		break;
	}
}

__inline void process_desktop_tracker_command(
	current_event *p_current_event,
	channel_info  *p_current_voice,
	tune_info     *p_current_positions,
	long          *p_phase_incrementors,
	mod_details   *p_module,
	unsigned int  *p_periods,
	yn            on_event,
	long          p_sample_rate)
{
	long *left_channel_multiplier_ptr;
	long *right_channel_multiplier_ptr;
	long *phase_incrementors_ptr;
	unsigned char temporary_note;
	unsigned char command[4];
	unsigned char data[4];
	unsigned int *periods_ptr;
	int foo;
	int bar;

	command[0] = p_current_event->command;
	command[1] = p_current_event->command1;
	command[2] = p_current_event->command2;
	command[3] = p_current_event->command3;
	data[0] = p_current_event->data;
	data[1] = p_current_event->data1;
	data[2] = p_current_event->data2;
	data[3] = p_current_event->data3;

	for (foo=0; foo<4; foo++)
	{
		switch (command[foo])
		{
		case VOLUME_COMMAND_DSKT:
			if (on_event == YES)
				p_current_voice->volume = ((data[foo] + 1) << 1) - 1;
			break;

		case SPEED_COMMAND_DSKT:
			if (data[foo] /* ensure an "S00" command does not hang player! */
		    	&& (on_event == YES))
				p_current_positions->speed = data[foo];
			break;

		case STEREO_COMMAND_DSKT:
			if (on_event == YES) {
				left_channel_multiplier_ptr = left_channel_multiplier;
				right_channel_multiplier_ptr = right_channel_multiplier;
				left_channel_multiplier_ptr += (data[foo] - 1);
				right_channel_multiplier_ptr += (data[foo] - 1);
				p_current_voice->left_channel_multiplier = *left_channel_multiplier_ptr;
				p_current_voice->right_channel_multiplier = *right_channel_multiplier_ptr;
			}
			break;

		case VOLSLIDE_COMMAND_DSKT:
			bar = (signed char)data[foo] << 1;
			if (bar > 0) {
				if ((255 - p_current_voice->volume) > bar)
					p_current_voice->volume += bar;
				else
					p_current_voice->volume = 255;
			} else if (bar < 0) {
				if (p_current_voice->volume >= bar)
					p_current_voice->volume += bar; /* is -ve value ! */
				else
					p_current_voice->volume = 0;
			}
			break;

		case PORTUP_COMMAND_DSKT:
			p_current_voice->period -= data[foo];
			if (p_current_voice->period < 0x50)
				p_current_voice->period = 0x50;
			phase_incrementors_ptr = p_phase_incrementors;
			phase_incrementors_ptr += p_current_voice->period;
			p_current_voice->phase_increment = *phase_incrementors_ptr;
			break;

		case PORTDOWN_COMMAND_DSKT:
			p_current_voice->period += data[foo];
			if (p_current_voice->period > 0x3f0)
				p_current_voice->period = 0x3f0;
			phase_incrementors_ptr = p_phase_incrementors;
			phase_incrementors_ptr += p_current_voice->period;
			p_current_voice->phase_increment = *phase_incrementors_ptr;
			break;

		case TONEPORT_COMMAND_DSKT:
			if (data[foo]) {
				p_current_voice->last_data_byte = data[foo];
			} else {
				data[foo] = p_current_voice->last_data_byte;
			}
			if (p_current_voice->period < p_current_voice->target_period) {
				p_current_voice->period += data[foo];
				if (p_current_voice->period > p_current_voice->target_period) {
					p_current_voice->period = p_current_voice->target_period;
				}
			} else {
				p_current_voice->period -= data[foo];
				if (p_current_voice->period < p_current_voice->target_period) {
					p_current_voice->period = p_current_voice->target_period;
				}
			}
			phase_incrementors_ptr = p_phase_incrementors;
			phase_incrementors_ptr += p_current_voice->period;
			p_current_voice->phase_increment = *phase_incrementors_ptr;
			break;

		case ARPEGGIO_COMMAND_DSKT:
			if (data[foo]) {
				if (p_current_voice->arpeggio_counter == 0)
					temporary_note = p_current_voice->note_currently_playing;
				else if (p_current_voice->arpeggio_counter == 1) {
					temporary_note = p_current_voice->note_currently_playing +
					((data[foo] & 0xf0) >> 4);
	
					if (temporary_note > 36)
						temporary_note = p_current_voice->note_currently_playing;
					} else if (p_current_voice->arpeggio_counter == 2) {
						temporary_note = p_current_voice->note_currently_playing +
						(data[foo] & 0xf);
	
					if (temporary_note > 36)
						temporary_note = p_current_voice->note_currently_playing;
				}
	
				if (++(p_current_voice->arpeggio_counter) == 3)
					p_current_voice->arpeggio_counter = 0;
	
				phase_incrementors_ptr = p_phase_incrementors;
				periods_ptr = p_periods;

				periods_ptr += (temporary_note + 13);
				phase_incrementors_ptr += *periods_ptr;

				p_current_voice->period = *periods_ptr;
				p_current_voice->phase_increment = *phase_incrementors_ptr;
			}
			break;

		case JUMP_COMMAND_DSKT:
			/* move position to last event before the requested sequence position */
			if (on_event == YES) {
				p_current_positions->position_in_sequence = data[foo] - 1;
				p_current_positions->position_in_pattern =
				p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]] - 1;
			}
			break;

		case SETFINETEMPO_COMMAND_DSKT:
			if (on_event == YES && data[foo])
				p_current_positions->sps_per_tick = (p_sample_rate << 8)/(data[foo]);
			break;

		case FINEPORTAMENTO_COMMAND_DSKT:
			if (on_event == YES && data[foo]) {
				bar = (unsigned char)data[foo];
				p_current_voice->period += bar;
				if (bar > 0) {
					if (p_current_voice->period > 0x3f0) {
						p_current_voice->period = 0x3f0;
					}
				} else {
					if (p_current_voice->period < 0x50) {
						p_current_voice->period = 0x50;
					}
				}
				phase_incrementors_ptr = p_phase_incrementors;
				phase_incrementors_ptr += p_current_voice->period;
				p_current_voice->phase_increment = *phase_incrementors_ptr;
			}
			break;

		case FINEVOLSLIDE_COMMAND_DSKT:
			if (on_event == YES && data[foo]) {
				bar = (signed char)data[foo] << 1;
				if (bar > 0) {
					if ((255 - p_current_voice->volume) > bar) {
						p_current_voice->volume += bar;
					} else {
						p_current_voice->volume = 255;
					}
				}
				else if (bar < 0) {
					if (p_current_voice->volume >= bar) {
						p_current_voice->volume += bar; /* is -ve value ! */
					} else {
						p_current_voice->volume = 0;
					}
				}
			}
			break;

#ifdef DEVELOPING
		default:
			if (on_event == YES) printf("| %2X %2X\n",command[foo],data[foo]);
#endif
		}
	} /* end for (foo) */
}

/* function write_audio_data            **
** write one tick's worth of audio data */

__inline return_status write_audio_data(
	output_api p_api,
	channel_info *p_voice_info,
	mod_details *p_module,
	mono_stereo p_stereo_mode,
	unsigned char p_volume,
	format p_sample_format,
	char p_buffer_shifter,
	void *p_ah_ptr,
	long p_nframes)
{
	return_status retcode = SUCCESS;
	static int nframes_fraction = 0;
	static long bufptr = 0;
	int ch;
	long nframes, frames_written;

	nframes = p_nframes >> 8;
	nframes_fraction += p_nframes - (nframes << 8);
	if (nframes_fraction > 256) {
		nframes++;
		nframes_fraction -= 256;
	}

	while (nframes) {
		for (ch=0; ch<p_module->num_channels; ch++) {
			write_channel_audio_data(
				ch,
				p_voice_info + ch,
				nframes>((BUF_SIZE - bufptr)>>p_buffer_shifter)?((BUF_SIZE - bufptr)>>p_buffer_shifter):nframes,
				p_buffer_shifter,
				((bufptr>>(p_buffer_shifter-1))*p_module->num_channels)+(ch<<1), /* offset into channel buffer in units (not bytes) */
				p_stereo_mode,
				p_volume,
				(p_module->num_channels<<1) - 2); /* channel buffer stride length (for interleaved channels) */
			frames_written = nframes>((BUF_SIZE - bufptr)>>p_buffer_shifter)?((BUF_SIZE - bufptr)>>p_buffer_shifter):nframes;
		}
		bufptr += frames_written<<p_buffer_shifter;
		if (bufptr == BUF_SIZE) {
			retcode = output_data(
				p_api,
				p_buffer_shifter,
				p_sample_format,
				p_stereo_mode,
				p_ah_ptr,
				p_module->num_channels);
			bufptr = 0;
		}
		nframes -= frames_written;
	}
	
	return (retcode);
}

/* function write_channel_audio_data                 **
** write nframes worth of audio data for one channel */

__inline void write_channel_audio_data(
	int p_ch,
	channel_info *p_voice_info,
	long p_nframes,
	char p_buffer_shifter,
	long p_bufptr,
	mono_stereo p_stereo_mode,
	unsigned char p_volume,
	int stridelen)
{
	long *bptr = channel_buffer + p_bufptr;
	unsigned char mlaw;
	long i = p_nframes, rval, lval;
	void *sptr;

	while (i--) {
		if (p_voice_info->channel_currently_playing == YES) {
			sptr = p_voice_info->sample_pointer + p_voice_info->phase_accumulator;
			mlaw = *(unsigned char *)sptr;

			/* adjust volume (NOTE: logarithmic) */
			if (mlaw > (255 - p_voice_info->volume)) mlaw -= (255 - p_voice_info->volume);
			else mlaw = 0;

			/* increment phase accumulator */
			p_voice_info->phase_acc_fraction += p_voice_info->phase_increment;
			p_voice_info->phase_accumulator += p_voice_info->phase_acc_fraction >> 16;
			p_voice_info->phase_acc_fraction -= (p_voice_info->phase_acc_fraction >> 16) << 16;

			/* end of sample? */
			if (p_voice_info->sample_length <= p_voice_info->phase_accumulator)
				/* if sample repeats then set accumulator back to repeat offset */
				if (p_voice_info->sample_repeats == YES) p_voice_info->phase_accumulator -= p_voice_info->repeat_length;
				else p_voice_info->channel_currently_playing = NO;

			/* convert mu-law to linear signed */
			rval = lval = *(log_lin_tab + mlaw);

			/* adjust volume and set stereo */
			if (p_stereo_mode == STEREO) {
				lval = (lval * p_voice_info->left_channel_multiplier * p_volume) >> 16;
				rval = (rval * p_voice_info->right_channel_multiplier * p_volume) >> 16;
			} else lval = (lval * p_volume) >> 9;
		} else lval = rval = 0; /* silence is golden :) */

		/* copy values to the channel buffer */
		*(bptr++) = lval;
		*(bptr++) = rval;
		bptr += stridelen;
	}
}

/* function output_data                                                **
** merge the audio data from the channel buffer into the output buffer **
** and send it to the audio device using the appropriate api           */

__inline return_status output_data(
	output_api p_api,
	char p_buffer_shifter,
	format p_sample_format,
	mono_stereo p_stereo_mode,
	void *p_ah_ptr,
	long p_num_channels)
{
	unsigned char *obptr = audio_buffer;
	long *cbptr = channel_buffer;
	int nframes = BUF_SIZE >> p_buffer_shifter, i, j, len, err;
	long lval, rval;

	int v_audio_fd = *(int *)p_ah_ptr;

#ifdef HAVE_LIBASOUND
	snd_pcm_t *v_pb_handle = *(snd_pcm_t **)p_ah_ptr;
#endif

#ifdef HAVE_LIBARTSC
	arts_stream_t v_stream = *(arts_stream_t *)p_ah_ptr;
#endif

	i = nframes = BUF_SIZE >> p_buffer_shifter;

	/*
	** mix the data from the channel buffer into the output buffer
	** the channel buffer data is interleaved short ints:
	**   chan 0 left, chan 0 right,
	**   chan 1 left, chan 1 right,
	**   chan 2 left, chan 2 right,
	**   chan 3 left, chan 3 right,
	**   etc.
	*/

    switch (p_sample_format) {
	case BITS_16_SIGNED_LITTLE_ENDIAN:
		while (i--) {
			lval = rval = 0;
			for (j=0; j<p_num_channels; j++) {
				lval+= *(cbptr++);	
				rval+= *(cbptr++);
			}
			if (lval > 32767) lval = 32767;
			if (lval < -32768) lval = -32768;
			if (rval > 32767) rval = 32767;
			if (rval < -32768) rval = -32768;
			*(obptr++) = (unsigned char)(lval & 0xff);
			*(obptr++) = (unsigned char)((lval >> 8) & 0xff);
			if (p_stereo_mode == STEREO) {
				*(obptr++) = (unsigned char)(rval & 0xff);
				*(obptr++) = (unsigned char)((rval >> 8) & 0xff);
			}
		}
		break;

	case BITS_16_SIGNED_BIG_ENDIAN:
		while (i--) {
			lval = rval = 0;
			for (j=0; j<p_num_channels; j++) {
				lval+= *(cbptr++);	
				rval+= *(cbptr++);
			}
			if (lval > 32767) lval = 32767;
			if (lval < -32768) lval = -32768;
			if (rval > 32767) rval = 32767;
			if (rval < -32768) rval = -32768;
			*(obptr++) = (unsigned char)((lval >> 8) & 0xff);
			*(obptr++) = (unsigned char)(lval & 0xff);
			if (p_stereo_mode == STEREO) {
				*(obptr++) = (unsigned char)((rval >> 8) & 0xff);
				*(obptr++) = (unsigned char)(rval & 0xff);
			}
		}
		break;

	case BITS_16_UNSIGNED_LITTLE_ENDIAN:
		while (i--) {
			lval = rval = 0;
			for (j=0; j<p_num_channels; j++) {
				lval+= *(cbptr++);	
				rval+= *(cbptr++);
			}
			if (lval > 32767) lval = 32767;
			if (lval < -32768) lval = -32768;
			if (rval > 32767) rval = 32767;
			if (rval < -32768) rval = -32768;
			*(obptr++) = (unsigned char)((lval ^ 32768) & 0xff);
			*(obptr++) = (unsigned char)(((lval ^ 32768) >> 8) & 0xff);
			if (p_stereo_mode == STEREO) {
				*(obptr++) = (unsigned char)((rval ^ 32768) & 0xff);
				*(obptr++) = (unsigned char)(((rval ^ 32768) >> 8) & 0xff);
			}
		}
		break;

	case BITS_16_UNSIGNED_BIG_ENDIAN:
		while (i--) {
			lval = rval = 0;
			for (j=0; j<p_num_channels; j++) {
				lval+= *(cbptr++);	
				rval+= *(cbptr++);
			}
			if (lval > 32767) lval = 32767;
			if (lval < -32768) lval = -32768;
			if (rval > 32767) rval = 32767;
			if (rval < -32768) rval = -32768;
			*(obptr++) = (unsigned char)(((lval ^ 32768) >> 8) & 0xff);
			*(obptr++) = (unsigned char)((lval ^ 32768) & 0xff);
			if (p_stereo_mode == STEREO) {
				*(obptr++) = (unsigned char)(((rval ^ 32768) >> 8) & 0xff);
				*(obptr++) = (unsigned char)((rval ^ 32768) & 0xff);
			}
		}
		break;

	case BITS_8_SIGNED:
		while (i--) {
			lval = rval = 0;
			for (j=0; j<p_num_channels; j++) {
				lval+= *(cbptr++);	
				rval+= *(cbptr++);
			}
			if (lval > 32767) lval = 32767;
			if (lval < -32768) lval = -32768;
			if (rval > 32767) rval = 32767;
			if (rval < -32768) rval = -32768;
			*(obptr++) = (char)((lval >> 8) & 0xff);
			if (p_stereo_mode == STEREO) *(obptr++) = (char)((rval >> 8) & 0xff);
		}
		break;

	case BITS_8_UNSIGNED:
		while (i--) {
			lval = rval = 0;
			for (j=0; j<p_num_channels; j++) {
				lval+= *(cbptr++);	
				rval+= *(cbptr++);
			}
			if (lval > 32767) lval = 32767;
			if (lval < -32768) lval = -32768;
			if (rval > 32767) rval = 32767;
			if (rval < -32768) rval = -32768;
			*(obptr++) = (unsigned char)(((lval ^ 32768) >> 8) & 0xff);
			if (p_stereo_mode == STEREO) *(obptr++) = (unsigned char)(((rval ^ 32768) >> 8) & 0xff);
		}
	}

	/* send the data to the audio device */
	if (p_api == OSS) {
		if ((len = write(v_audio_fd, audio_buffer, BUF_SIZE)) == -1) {
			perror("audio write");
			return (AUDIO_WRITE_ERROR); /* bomb out */
		}
	} else if (p_api == ALSA) {
#ifdef HAVE_LIBASOUND
		if ((err = snd_pcm_writei (v_pb_handle, audio_buffer, nframes)) != nframes) {
			fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (err));
			return (ALSA_ERROR); /* bomb out */
		}
#else
		1; /* this should not happen */
#endif
	} else /* arts */ {
#ifdef HAVE_LIBARTSC
		if ((len = arts_write(v_stream, audio_buffer, BUF_SIZE)) < 0) {
			fprintf(stderr, "arts_write error: %s\n", arts_error_text(len));
			return (ARTS_ERROR); /* bomb out */
		}
#else
		1; /* this should not happen */
#endif
	}
	return (SUCCESS);
}
