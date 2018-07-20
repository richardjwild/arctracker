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
#include "heap.h"
#include "effects.h"

voice_t *initialise_voices(const module_t *module);

void decode_next_events(const module_t *module, channel_event_t *decoded_events);

void silence_channel(voice_t *voice);

void reset_gain_to_sample_default(voice_t *voice, sample_t sample);

void set_portamento_target(channel_event_t event, sample_t sample, voice_t *voice);

void trigger_new_note(channel_event_t event, sample_t sample, voice_t *voice);

void process_commands(channel_event_t *event, voice_t *voice, bool on_event);

bool portamento(channel_event_t event);

bool sample_out_of_range(channel_event_t event, module_t module);

void play_module(module_t *module, audio_api_t audio_api)
{
    args_t config = configuration();
    initialise_audio(audio_api, module->num_channels);
    set_master_gain(config.volume);
    set_clock(module->initial_speed, audio_api.sample_rate);
    initialise_sequence(module);
    configure_console(config.pianola, module);
    voice_t *voices = initialise_voices(module);
    channel_event_t events[MAX_CHANNELS];

    while (!looped_yet() || config.loop_forever)
    {
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
    send_remaining_audio();
}

inline
bool sample_out_of_range(const channel_event_t event, const module_t module)
{
    return event.sample > module.num_samples;
}

inline
bool portamento(const channel_event_t event)
{
    const effect_t *effects = event.effects;
    for (int effect_no = 0; effect_no < 4; effect_no++)
    {
        if (effects[effect_no].command == TONE_PORTAMENTO)
            return true;
    }
    return false;
}

inline
void silence_channel(voice_t *voice)
{
    voice->channel_playing = false;
}

inline
void reset_gain_to_sample_default(voice_t *voice, sample_t sample)
{
    voice->gain = sample.default_gain;
}

voice_t *initialise_voices(const module_t *module)
{
    voice_t *voices = allocate_array(module->num_channels, sizeof(voice_t));
    for (int channel = 0; channel < module->num_channels; channel++)
    {
        voices[channel].channel_playing = false;
        voices[channel].panning = module->initial_panning[channel] - 1;
    }
    return voices;
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
