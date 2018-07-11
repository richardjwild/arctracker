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
#include "play_mod.h"
#include "config.h"
#include "error.h"
#include "write_audio.h"
#include "gain.h"
#include "period.h"
#include "configuration.h"
#include "console.h"
#include "clock.h"
#include "audio_api.h"

void initialise_values(
        positions_t *p_current_positions,
        voice_t *p_voice_info,
        const module_t *p_module,
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

void process_commands(
        channel_event_t *p_current_event,
        voice_t *p_current_voice,
        positions_t *p_current_positions,
        const module_t *p_module,
        bool on_event);

static args_t config;

static inline
bool portamento(const channel_event_t event)
{
	return event.effects[0].command == TONE_PORTAMENTO;
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

	bool looped_yet = false;

	initialise_values(
		&current_positions,
		voice_info,
		p_module,
		audio_api.sample_rate);

	initialise_audio(audio_api, p_module->num_channels);
	set_master_gain(config.volume);
    set_clock(p_module->initial_speed, audio_api.sample_rate);
    configure_console(config.pianola, p_module);

	/* loop through whole tune */
	do {
        clock_tick();
		if (new_event())
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

			pianola_roll(&current_positions, current_pattern_line);
		}
        for (int channel = 0; channel < p_module->num_channels; channel++)
        {
            channel_event_t event = current_pattern_line[channel];
            sample_t sample = p_module->samples[event.sample - 1];
            voice_t voice = voice_info[channel];
            if (new_event())
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
            process_commands(
                    &event,
                    &voice,
                    &current_positions,
                    p_module,
                    new_event());
            voice_info[channel] = voice;
        }

        write_audio_data(voice_info);
	}
	while (!looped_yet || config.loop_forever);
    send_remaining_audio();
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
	long p_sample_rate)
{
	int channel;

	p_current_positions->position_in_sequence = 0;
	p_current_positions->position_in_pattern = -1;
	p_current_positions->counter = p_module->initial_speed - 1;
	p_current_positions->speed = p_module->initial_speed;
	p_current_positions->pattern_line_ptr = p_module->patterns[p_module->sequence[0]];

	/* initialise voice info: all voices silent and set initial stereo positions */
	for (channel = 0; channel < p_module->num_channels; channel++) {
		p_voice_info[channel].channel_playing = false;
		p_voice_info[channel].panning = p_module->default_channel_stereo[channel] - 1;
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

		output_new_position(p_current_positions);
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

	pattern_line_ptr = p_current_positions->pattern_line_ptr;

	for (channel = 0, current_pattern_line_ptr=p_current_pattern_line;
	channel < p_module->num_channels;
	channel++, current_pattern_line_ptr++) {
		size_t event_bytes = p_module->decode_event(pattern_line_ptr, current_pattern_line_ptr);
		pattern_line_ptr += event_bytes;
	}

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

void process_commands(
        channel_event_t *p_current_event,
        voice_t *p_current_voice,
        positions_t *p_current_positions,
        const module_t *p_module,
        bool on_event)
{
	unsigned char temporary_note;
	int bar;

	for (int effect_no = 0; effect_no < 4; effect_no++)
	{
	    effect_t effect = p_current_event->effects[effect_no];
		switch (effect.command)
		{
        case SET_VOLUME_TRACKER:
            if (on_event)
                p_current_voice->gain = effect.data;
            break;

		case SET_VOLUME_DESKTOP_TRACKER:
			if (on_event)
				p_current_voice->gain = ((effect.data + 1) << 1) - 1;
			break;

		case SET_TEMPO:
			if (on_event && effect.data) /* ensure an "S00" command does not hang player! */
				set_ticks_per_event(effect.data);
			break;

		case SET_TRACK_STEREO:
			if (on_event) {
			    p_current_voice->panning = effect.data - 1;
			}
			break;

        case VOLUME_SLIDE_UP:
            if ((255 - p_current_voice->gain) > effect.data)
                p_current_voice->gain += effect.data;
            else
                p_current_voice->gain = 255;
            break;

        case VOLUME_SLIDE_DOWN:
            if (p_current_voice->gain >= effect.data)
                p_current_voice->gain -= effect.data;
            else
                p_current_voice->gain = 0;
            break;

        case VOLUME_SLIDE:
			bar = (signed char)effect.data << 1;
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

		case PORTAMENTO_UP:
			p_current_voice->period -= effect.data;
			if (p_current_voice->period < 0x50)
				p_current_voice->period = 0x50;
			break;

		case PORTAMENTO_DOWN:
			p_current_voice->period += effect.data;
			if (p_current_voice->period > 0x3f0)
				p_current_voice->period = 0x3f0;
			break;

		case TONE_PORTAMENTO:
			if (effect.data) {
				p_current_voice->last_data_byte = effect.data;
			} else {
				effect.data = p_current_voice->last_data_byte;
			}
			if (p_current_voice->period < p_current_voice->target_period) {
				p_current_voice->period += effect.data;
				if (p_current_voice->period > p_current_voice->target_period) {
					p_current_voice->period = p_current_voice->target_period;
				}
			} else {
				p_current_voice->period -= effect.data;
				if (p_current_voice->period < p_current_voice->target_period) {
					p_current_voice->period = p_current_voice->target_period;
				}
			}
			break;

		case ARPEGGIO:
			if (effect.data) {
				if (p_current_voice->arpeggio_counter == 0)
					temporary_note = p_current_voice->note_currently_playing;
				else if (p_current_voice->arpeggio_counter == 1) {
					temporary_note = p_current_voice->note_currently_playing +
					((effect.data & 0xf0) >> 4);

					if (0 > temporary_note || temporary_note > 61)
						temporary_note = p_current_voice->note_currently_playing;
					} else if (p_current_voice->arpeggio_counter == 2) {
						temporary_note = p_current_voice->note_currently_playing +
						(effect.data & 0xf);

					if (0 > temporary_note || temporary_note > 61)
						temporary_note = p_current_voice->note_currently_playing;
				}

				if (++(p_current_voice->arpeggio_counter) == 3)
					p_current_voice->arpeggio_counter = 0;

				p_current_voice->period = period_for_note(temporary_note);
			}
			break;

        case BREAK_PATTERN:
            /* jog position (in pattern) to last event */
            if (on_event)
                p_current_positions->position_in_pattern =
                        p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]] - 1;
            break;

        case JUMP_TO_POSITION:
			/* move position to last event before the requested sequence position */
			if (on_event) {
				p_current_positions->position_in_sequence = effect.data - 1;
				p_current_positions->position_in_pattern =
				p_module->pattern_length[p_module->sequence[p_current_positions->position_in_sequence]] - 1;
			}
			break;

		case SET_TEMPO_FINE:
			if (on_event && effect.data)
                set_ticks_per_second(effect.data);
			break;

		case PORTAMENTO_FINE:
			if (on_event && effect.data) {
				bar = (unsigned char)effect.data;
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

		case VOLUME_SLIDE_FINE:
			if (on_event && effect.data) {
				bar = (signed char)effect.data << 1;
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
		}
	} /* end for (foo) */
}
