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

#include <stdbool.h>
#include "arctracker.h"
#include "play_mod.h"
#include "config.h"
#include "error.h"
#include "write_audio.h"
#include "gain.h"
#include "period.h"
#include "configuration.h"

char *notes[] = {"---",
	"C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
	"C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
	"C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3"};

char alphanum[] = {'-',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z'};

void initialise_values(
        positions_t *p_current_positions,
        voice_t *p_voice_info,
        const module_t *p_module,
        bool p_pianola,
        long p_sample_rate);

bool update_counters(
        positions_t *p_current_positions,
        const module_t *p_module);

void get_current_pattern_line(
        positions_t *p_current_positions,
        const module_t *p_module,
        channel_event_t *p_current_pattern_line);

void silence_channel(voice_t *voice);

void reset_gain_to_sample_default(voice_t *voice, sample_t sample);

void set_portamento_target(channel_event_t event, sample_t sample, voice_t *voice);

void trigger_new_note(channel_event_t event, sample_t sample, voice_t *voice);

void process_tracker_command(
        channel_event_t *p_current_event,
        voice_t *p_current_voice,
        positions_t *p_current_positions,
        const module_t *p_module,
        bool on_event);

void process_desktop_tracker_command(
        channel_event_t *p_current_event,
        voice_t *p_current_voice,
        positions_t *p_current_positions,
        const module_t *p_module,
        bool on_event,
        long p_sample_rate);

static args_t config;

static inline
bool portamento(const channel_event_t event)
{
	return event.command0 == TONEPORT_COMMAND_DSKT;
}

static inline
bool sample_out_of_range(const channel_event_t event, const module_t module)
{
	return event.sample > module.num_samples;
}

void play_module(const module_t *p_module, audio_api_t audio_api)
{
    config = configuration();
	positions_t current_positions;
	channel_event_t current_pattern_line[MAX_CHANNELS];
	voice_t voice_info[MAX_CHANNELS];
    int nframes_fraction = 0;

	bool looped_yet = false;

	initialise_values(
		&current_positions,
		voice_info,
		p_module,
		configuration().pianola,
		audio_api.sample_rate);

	initialise_audio(audio_api, p_module->num_channels);
	set_master_gain(config.volume);

	/* loop through whole tune */
	do {
	    current_positions.counter++;
	    bool new_event = (current_positions.counter == current_positions.speed);
		if (new_event)
		{
			/* new event. update counters: current position in pattern, position in sequence */
			looped_yet = update_counters(
				&current_positions,
				p_module);

			/* we have a new pattern line to process */
			get_current_pattern_line(
				&current_positions,
				p_module,
				current_pattern_line);
		}
        for (int channel = 0; channel < p_module->num_channels; channel++)
        {
            channel_event_t event = current_pattern_line[channel];
            sample_t sample = p_module->samples[event.sample - 1];
            voice_t voice = voice_info[channel];
            if (new_event)
            {
                if (event.note)
                {
                    if (portamento(event))
                        set_portamento_target(event, sample, &voice);
                    else if (sample_out_of_range(event, *p_module))
                        silence_channel(&voice);
                    else
                        trigger_new_note(event, sample, &voice);
                }
                else if (event.sample)
                {
                    reset_gain_to_sample_default(&voice, sample);
                }
            }
            if (p_module->format == TRACKER)
            {
                process_tracker_command(
                        &event,
                        &voice,
                        &current_positions,
                        p_module,
                        new_event);
            }
            else
            {
                process_desktop_tracker_command(
                        &event,
                        &voice,
                        &current_positions,
                        p_module,
                        new_event,
                        audio_api.sample_rate);
            }
            voice_info[channel] = voice;
        }

        int extra_frame = 0;
        nframes_fraction += current_positions.sps_per_tick;
        nframes_fraction -= current_positions.sps_per_tick & 0xffffffffffffff00;
        if (nframes_fraction > 256) {
            extra_frame = 1;
            nframes_fraction -= 256;
        }

        /* write one tick's worth of audio data */
		write_audio_data(voice_info, (current_positions.sps_per_tick >> 8) + extra_frame);
	}
	while (!looped_yet || config.loop_forever);
    send_remaining_audio();

	if (!config.pianola)
		printf("\n");
}

void silence_channel(voice_t *voice)
{
	voice->channel_playing = false;
}

void reset_gain_to_sample_default(voice_t *voice, sample_t sample)
{
	voice->gain = sample.default_gain;
}

/* initialise_values function.                    *
 * Set up values in preparation for player start. */

void initialise_values(
	positions_t *p_current_positions,
	voice_t *p_voice_info,
	const module_t *p_module,
	bool p_pianola,
	long p_sample_rate)
{
	int channel;

	p_current_positions->position_in_sequence = 0;
	p_current_positions->position_in_pattern = -1;
	p_current_positions->counter = p_module->initial_speed - 1;
	p_current_positions->speed = p_module->initial_speed;
	p_current_positions->pattern_line_ptr = p_module->patterns[p_module->sequence[0]];

#ifdef DEVELOPING
	if (p_pianola)
		printf("Pianola mode on\n");
	else
		printf("Pianola mode off\n");
#endif

	/* initialise voice info: all voices silent and set initial stereo positions */
	for (channel = 0; channel < p_module->num_channels; channel++) {
		p_voice_info[channel].channel_playing = false;
		p_voice_info[channel].panning = p_module->default_channel_stereo[channel] - 1;
	}

	if (!p_pianola) {
		printf("Playing position 1 of %ld", p_module->tune_length);
		fflush(stdout);
	}

	p_current_positions->sps_per_tick = (p_sample_rate << 8)/50;
}

/* update_counters function.                                         *
 * Called every n tracker periods where n is the current tune speed; *
 * updates position in pattern and position in sequence counters.    */

bool update_counters(
	positions_t *p_current_positions,
	const module_t *p_module)
{
	bool looped_yet = false;

	p_current_positions->counter = 0;
	if (++(p_current_positions->position_in_pattern) ==
	p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]]) {
		if (++p_current_positions->position_in_sequence == p_module->tune_length) {
			p_current_positions->position_in_sequence = 0;
			looped_yet = true;
		}
		p_current_positions->position_in_pattern = 0;

		int pattern = p_module->sequence[p_current_positions->position_in_sequence];
		p_current_positions->pattern_line_ptr = p_module->patterns[pattern];

		if (!config.pianola) {
			printf(
				"%cPlaying position %d of %ld ",
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

void get_current_pattern_line(
	positions_t *p_current_positions,
	const module_t *p_module,
	channel_event_t *p_current_pattern_line)
{
	int channel;
	void *pattern_line_ptr;
	channel_event_t *current_pattern_line_ptr;

	if (config.pianola)
		printf(
			"%2d %2d | ",
			p_current_positions->position_in_sequence,
			p_current_positions->position_in_pattern);

	pattern_line_ptr = p_current_positions->pattern_line_ptr;

	for (channel = 0, current_pattern_line_ptr=p_current_pattern_line;
	channel < p_module->num_channels;
	channel++, current_pattern_line_ptr++) {
		size_t event_bytes = p_module->decode_event(pattern_line_ptr, current_pattern_line_ptr);
		pattern_line_ptr += event_bytes;
		if (config.pianola) {
			printf(
					"%s %c%c%X%X | ",
					notes[current_pattern_line_ptr->note],
					alphanum[current_pattern_line_ptr->sample],
					alphanum[current_pattern_line_ptr->command0 + 1],
					(current_pattern_line_ptr->data0 >> 4) & 0xf,
					current_pattern_line_ptr->data0 & 0xf);
		}
	}
	if (config.pianola)
		printf("\n");

	/* remember pattern line address for next event */
	p_current_positions->pattern_line_ptr = pattern_line_ptr;
}

void set_portamento_target(channel_event_t event, sample_t sample, voice_t *voice)
{
    voice->target_period = period_for_note(event.note + sample.transpose);
}

void trigger_new_note(channel_event_t event, sample_t sample, voice_t *voice)
{
	voice->channel_playing = true;
	voice->sample_pointer = sample.sample_data;
	voice->phase_accumulator = 0.0;
	voice->arpeggio_counter = 0;
	voice->note_currently_playing = event.note + sample.transpose;
	voice->period = period_for_note(voice->note_currently_playing);
	voice->target_period = voice->period;
	voice->gain = sample.default_gain;
	voice->sample_repeats = sample.repeats;
	voice->repeat_length = sample.repeat_length;
	voice->sample_end = voice->sample_repeats
			? sample.repeat_offset + sample.repeat_length
			: sample.sample_length;
}

/* process_tracker_command function.  *
 * process a tracker command.         */

void process_tracker_command(
	channel_event_t *p_current_event,
	voice_t *p_current_voice,
	positions_t *p_current_positions,
	const module_t *p_module,
	bool on_event)
{
	unsigned char temporary_note;

	switch (p_current_event->command0) {
	case VOLUME_COMMAND:
		if (on_event)
			p_current_voice->gain = p_current_event->data0;
		break;

	case SPEED_COMMAND:
		if (on_event && p_current_event->data0) /* ensure an "S00" command does not hang player! */
			p_current_positions->speed = p_current_event->data0;
		break;

	case STEREO_COMMAND:
		if (on_event) {
		    p_current_voice->panning = p_current_event->data0 - 1;
		}
		break;

	case VOLSLIDEUP_COMMAND:
		if ((255 - p_current_voice->gain) > p_current_event->data0)
			p_current_voice->gain += p_current_event->data0;
		else
			p_current_voice->gain = 255;
		break;

	case VOLSLIDEDOWN_COMMAND:
		if (p_current_voice->gain >= p_current_event->data0)
			p_current_voice->gain -= p_current_event->data0;
		else
			p_current_voice->gain = 0;
		break;

	case PORTUP_COMMAND:
		p_current_voice->period -= p_current_event->data0;
		if (p_current_voice->period < 0x50)
			p_current_voice->period = 0x50;
		break;

	case PORTDOWN_COMMAND:
		p_current_voice->period += p_current_event->data0;
		if (p_current_voice->period > 0x3f0)
			p_current_voice->period = 0x3f0;
		break;

	case TONEPORT_COMMAND_DSKT:
		if (p_current_event->data0) {
			p_current_voice->last_data_byte = p_current_event->data0;
		} else {
			p_current_event->data0 = p_current_voice->last_data_byte;
		}
		if (p_current_voice->period < p_current_voice->target_period) {
			p_current_voice->period += p_current_event->data0;
			if (p_current_voice->period > p_current_voice->target_period) {
				p_current_voice->period = p_current_voice->target_period;
			}
		} else {
			p_current_voice->period -= p_current_event->data0;
			if (p_current_voice->period < p_current_voice->target_period) {
				p_current_voice->period = p_current_voice->target_period;
			}
		}
		break;

	case ARPEGGIO_COMMAND:
		if (p_current_event->data0) {
			if (p_current_voice->arpeggio_counter == 0)
				temporary_note = p_current_voice->note_currently_playing;
			else if (p_current_voice->arpeggio_counter == 1) {
				temporary_note = p_current_voice->note_currently_playing +
					         ((p_current_event->data0 & 0xf0) >> 4);

				if (temporary_note > 36)
					temporary_note = p_current_voice->note_currently_playing;
			}
			else if (p_current_voice->arpeggio_counter == 2) {
				temporary_note = p_current_voice->note_currently_playing +
								(p_current_event->data0 & 0xf);

				if (temporary_note > 36)
					temporary_note = p_current_voice->note_currently_playing;
			}

			if (++(p_current_voice->arpeggio_counter) == 3)
				p_current_voice->arpeggio_counter = 0;

			p_current_voice->period = period_for_note(temporary_note + 12);
		}
		break;

	case BREAK_COMMAND:
		/* jog position (in pattern) to last event */
		if (on_event)
			p_current_positions->position_in_pattern =
				p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]] - 1;
		break;

	case JUMP_COMMAND:
		/* move position to last event before the requested sequence position */
		if (on_event) {
			p_current_positions->position_in_sequence = p_current_event->data0 - 1;
			p_current_positions->position_in_pattern =
				p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]] - 1;
		}
		break;
	}
}

void process_desktop_tracker_command(
	channel_event_t *p_current_event,
	voice_t  *p_current_voice,
	positions_t     *p_current_positions,
	const module_t   *p_module,
	bool            on_event,
	long          p_sample_rate)
{
	unsigned char temporary_note;
	unsigned char command[4];
	unsigned char data[4];
	int foo;
	int bar;

	command[0] = p_current_event->command0;
	command[1] = p_current_event->command1;
	command[2] = p_current_event->command2;
	command[3] = p_current_event->command3;
	data[0] = p_current_event->data0;
	data[1] = p_current_event->data1;
	data[2] = p_current_event->data2;
	data[3] = p_current_event->data3;

	for (foo=0; foo<4; foo++)
	{
		switch (command[foo])
		{
		case VOLUME_COMMAND_DSKT:
			if (on_event)
				p_current_voice->gain = ((data[foo] + 1) << 1) - 1;
			break;

		case SPEED_COMMAND_DSKT:
			if (on_event && data[foo]) /* ensure an "S00" command does not hang player! */
				p_current_positions->speed = data[foo];
			break;

		case STEREO_COMMAND_DSKT:
			if (on_event) {
			    p_current_voice->panning = data[foo] - 1;
			}
			break;

		case VOLSLIDE_COMMAND_DSKT:
			bar = (signed char)data[foo] << 1;
			if (bar > 0) {
				if ((255 - p_current_voice->gain) > bar)
					p_current_voice->gain += bar;
				else
					p_current_voice->gain = 255;
			} else if (bar < 0) {
				if (p_current_voice->gain >= bar)
					p_current_voice->gain += bar; /* is -ve value ! */
				else
					p_current_voice->gain = 0;
			}
			break;

		case PORTUP_COMMAND_DSKT:
			p_current_voice->period -= data[foo];
			if (p_current_voice->period < 0x50)
				p_current_voice->period = 0x50;
			break;

		case PORTDOWN_COMMAND_DSKT:
			p_current_voice->period += data[foo];
			if (p_current_voice->period > 0x3f0)
				p_current_voice->period = 0x3f0;
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

				p_current_voice->period = period_for_note(temporary_note + 13);
			}
			break;

		case JUMP_COMMAND_DSKT:
			/* move position to last event before the requested sequence position */
			if (on_event) {
				p_current_positions->position_in_sequence = data[foo] - 1;
				p_current_positions->position_in_pattern =
				p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]] - 1;
			}
			break;

		case SETFINETEMPO_COMMAND_DSKT:
			if (on_event && data[foo])
				p_current_positions->sps_per_tick = (p_sample_rate << 8)/(data[foo]);
			break;

		case FINEPORTAMENTO_COMMAND_DSKT:
			if (on_event && data[foo]) {
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
			}
			break;

		case FINEVOLSLIDE_COMMAND_DSKT:
			if (on_event && data[foo]) {
				bar = (signed char)data[foo] << 1;
				if (bar > 0) {
					if ((255 - p_current_voice->gain) > bar) {
						p_current_voice->gain += bar;
					} else {
						p_current_voice->gain = 255;
					}
				}
				else if (bar < 0) {
					if (p_current_voice->gain >= bar) {
						p_current_voice->gain += bar; /* is -ve value ! */
					} else {
						p_current_voice->gain = 0;
					}
				}
			}
			break;

#ifdef DEVELOPING
		default:
			if (on_event) printf("| %2X %2X\n",command[foo],data[foo]);
#endif
		}
	} /* end for (foo) */
}
