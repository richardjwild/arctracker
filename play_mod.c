#include "arctracker.h"
#include "play_mod.h"
#include "config.h"
#include "error.h"
#include "write_audio.h"
#include "sequence.h"
#include "gain.h"
#include "period.h"
#include "configuration.h"
#include "console.h"
#include "clock.h"
#include "audio_api.h"

void initialise_voices(voice_t *voice, const module_t *module);

void decode_next_events(const module_t *module, channel_event_t *decoded_events);

void silence_channel(voice_t *voice);

void reset_gain_to_sample_default(voice_t *voice, sample_t sample);

void set_portamento_target(channel_event_t event, sample_t sample, voice_t *voice);

void trigger_new_note(channel_event_t event, sample_t sample, voice_t *voice);

void process_commands(channel_event_t *event, voice_t *voice, bool on_event);

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

void play_module(module_t *module, audio_api_t audio_api)
{
    config = configuration();
	channel_event_t events[MAX_CHANNELS];
	voice_t voices[MAX_CHANNELS];

    initialise_voices(voices, module);
	initialise_audio(audio_api, module->num_channels);
	set_master_gain(config.volume);
    set_clock(module->initial_speed, audio_api.sample_rate);
    initialise_sequence(module);
    configure_console(config.pianola, module);

	do {
        clock_tick();
		if (new_event())
		{
            decode_next_events(module, events);
			output_to_console(events);
		}
        for (int channel = 0; channel < module->num_channels; channel++)
        {
            channel_event_t event = events[channel];
            sample_t sample = module->samples[event.sample - 1];
            voice_t voice = voices[channel];
            if (new_event())
            {
                if (event.note)
                {
                    if (portamento(event))
                        set_portamento_target(event, sample, &voice);
                    else if (sample_out_of_range(event, *module))
                        silence_channel(&voice);
                    else
                        trigger_new_note(event, sample, &voice);
                }
                else if (event.sample)
                {
                    reset_gain_to_sample_default(&voice, sample);
                }
            }
            process_commands(&event, &voice, new_event());
            voices[channel] = voice;
        }

        write_audio_data(voices);
	}
	while (!looped_yet() || config.loop_forever);
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

void initialise_voices(voice_t *voice, const module_t *module)
{
	for (int channel = 0; channel < module->num_channels; channel++) {
		voice[channel].channel_playing = false;
		voice[channel].panning = module->default_channel_stereo[channel] - 1;
	}
}

void decode_next_events(const module_t *module, channel_event_t *decoded_events)
{
    next_event();
	for (int channel = 0; channel < module->num_channels; channel++)
	{
		size_t event_bytes = module->decode_event(pattern_line(), decoded_events + channel);
		advance_pattern_line(event_bytes);
	}
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

void process_commands(channel_event_t *event, voice_t *voice, bool on_event)
{
	int bar;

	for (int effect_no = 0; effect_no < 4; effect_no++)
	{
	    effect_t effect = event->effects[effect_no];
		switch (effect.command)
		{
        case SET_VOLUME_TRACKER:
            if (on_event)
                voice->gain = effect.data;
            break;

		case SET_VOLUME_DESKTOP_TRACKER:
			if (on_event)
				voice->gain = ((effect.data + 1) << 1) - 1;
			break;

		case SET_TEMPO:
			if (on_event && effect.data) /* ensure an "S00" command does not hang player! */
				set_ticks_per_event(effect.data);
			break;

		case SET_TRACK_STEREO:
			if (on_event) {
			    voice->panning = effect.data - 1;
			}
			break;

        case VOLUME_SLIDE_UP:
            if ((255 - voice->gain) > effect.data)
                voice->gain += effect.data;
            else
                voice->gain = 255;
            break;

        case VOLUME_SLIDE_DOWN:
            if (voice->gain >= effect.data)
                voice->gain -= effect.data;
            else
                voice->gain = 0;
            break;

        case VOLUME_SLIDE:
			bar = (signed char)effect.data << 1;
			if (bar > 0) {
				if ((255 - voice->gain) > bar)
					voice->gain += bar;
				else
					voice->gain = 255;
			} else if (bar < 0) {
				if (voice->gain >= bar)
					voice->gain += bar; /* is -ve value ! */
				else
					voice->gain = 0;
			}
			break;

		case PORTAMENTO_UP:
			voice->period -= effect.data;
			if (voice->period < 0x50)
				voice->period = 0x50;
			break;

		case PORTAMENTO_DOWN:
			voice->period += effect.data;
			if (voice->period > 0x3f0)
				voice->period = 0x3f0;
			break;

		case TONE_PORTAMENTO:
			if (effect.data) {
				voice->last_data_byte = effect.data;
			} else {
				effect.data = voice->last_data_byte;
			}
			if (voice->period < voice->target_period) {
				voice->period += effect.data;
				if (voice->period > voice->target_period) {
					voice->period = voice->target_period;
				}
			} else {
				voice->period -= effect.data;
				if (voice->period < voice->target_period) {
					voice->period = voice->target_period;
				}
			}
			break;

		case ARPEGGIO:
			if (effect.data) {
                int temporary_note;
                if (voice->arpeggio_counter == 0)
					temporary_note = voice->note_currently_playing;
				else if (voice->arpeggio_counter == 1) {
					temporary_note = voice->note_currently_playing +
					((effect.data & 0xf0) >> 4);

					if (0 > temporary_note || temporary_note > 61)
						temporary_note = voice->note_currently_playing;
					} else if (voice->arpeggio_counter == 2) {
						temporary_note = voice->note_currently_playing +
						(effect.data & 0xf);

					if (0 > temporary_note || temporary_note > 61)
						temporary_note = voice->note_currently_playing;
				}

				if (++(voice->arpeggio_counter) == 3)
					voice->arpeggio_counter = 0;

				voice->period = period_for_note(temporary_note);
			}
			break;

        case BREAK_PATTERN:
            if (on_event)
                break_to_next_position();
            break;

        case JUMP_TO_POSITION:
			if (on_event)
			    jump_to_position(effect.data);
			break;

		case SET_TEMPO_FINE:
			if (on_event && effect.data)
                set_ticks_per_second(effect.data);
			break;

		case PORTAMENTO_FINE:
			if (on_event && effect.data) {
				bar = (unsigned char)effect.data;
				voice->period += bar;
				if (bar > 0) {
					if (voice->period > 0x3f0) {
						voice->period = 0x3f0;
					}
				} else {
					if (voice->period < 0x50) {
						voice->period = 0x50;
					}
				}
			}
			break;

		case VOLUME_SLIDE_FINE:
			if (on_event && effect.data) {
				bar = (signed char)effect.data << 1;
				if (bar > 0) {
					if ((255 - voice->gain) > bar) {
						voice->gain += bar;
					} else {
						voice->gain = 255;
					}
				}
				else if (bar < 0) {
					if (voice->gain >= bar) {
						voice->gain += bar; /* is -ve value ! */
					} else {
						voice->gain = 0;
					}
				}
			}
			break;
		}
	}
}
