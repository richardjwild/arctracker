#include "play_mod.h"
#include "sequence.h"
#include "effects.h"
#include <config.h>
#include <audio/gain.h>
#include <pcm/mu_law.h>
#include <audio/period.h>
#include <audio/write_audio.h>
#include <chrono/clock.h>
#include <io/error.h>
#include <io/configuration.h>
#include <io/console.h>
#include <memory/heap.h>

static module_t module;
static args_t config;

voice_t *initialise_player(module_t *, audio_api_t);

bool writing_to_file(args_t);

voice_t *initialise_voices();

channel_event_t *decode_next_events();

void silence_channel(voice_t *);

void reset_gain_to_sample_default(voice_t *, sample_t);

void set_portamento_target(channel_event_t, sample_t, voice_t *);

void trigger_new_note(channel_event_t, sample_t, voice_t *);

bool portamento(channel_event_t);

bool sample_out_of_range(channel_event_t);

void output_one_tick(voice_t *, const channel_event_t *);

void update_voice_every_tick(channel_event_t, voice_t *);

void play_module(module_t *module, audio_api_t audio_api)
{
    voice_t *voices = initialise_player(module, audio_api);
    bool song_finished = false;
    channel_event_t *events = NULL;
    while (!song_finished)
    {
        clock_tick();
        if (new_event())
        {
            events = decode_next_events();
            output_to_console(events);
        }
        if (!looped_yet() || config.loop_forever)
            output_one_tick(voices, events);
        else
            song_finished = true;
    }
    send_remaining_audio();
    finish_console();
}

voice_t *initialise_player(module_t *module_p, const audio_api_t audio_api)
{
    module = *module_p;
    args_t config = configuration();
    initialise_audio(audio_api, module.num_channels);
    set_master_gain(config.volume);
    set_module_gain_characteristics(module.gain_curve, module.gain_maximum);
    set_clock(module.initial_speed, audio_api.sample_rate);
    initialise_sequence(module_p);
    configure_console(config.pianola, module_p);
    if (writing_to_file(config))
    {
        forbid_jumping_backwards();
    }
    return initialise_voices();
}

bool writing_to_file(args_t config)
{
    return config.output_filename != NULL;
}

void output_one_tick(voice_t *voices, const channel_event_t *events)
{
    for (int channel = 0; channel < module.num_channels; channel++)
    {
        channel_event_t event = events[channel];
        voice_t voice = voices[channel];
        update_voice_every_tick(event, &voice);
        voices[channel] = voice;
    }
    write_audio_data(voices);
}

void update_voice_every_tick(channel_event_t event, voice_t *voice)
{
    sample_t sample = module.samples[event.sample - 1];
    if (new_event())
    {
        if (event.note)
        {
            if (portamento(event))
                set_portamento_target(event, sample, voice);
            else if (sample_out_of_range(event))
                silence_channel(voice);
            else
                trigger_new_note(event, sample, voice);
        }
        else if (event.sample)
        {
            reset_gain_to_sample_default(voice, sample);
        }
        reset_arpeggiator(voice);
        handle_effects_on_event(&event, voice);
    }
    else
        handle_effects_off_event(&event, voice);
}

inline
bool sample_out_of_range(const channel_event_t event)
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
    voice->gain = get_internal_gain(sample.default_gain);
}

voice_t *initialise_voices()
{
    voice_t *voices = allocate_array(module.num_channels, sizeof(voice_t));
    for (int channel = 0; channel < module.num_channels; channel++)
    {
        voices[channel].channel_playing = false;
        voices[channel].arpeggiator_on = false;
        voices[channel].panning = module.initial_panning[channel] - 1;
        voices[channel].gain = INTERNAL_GAIN_MAX;
    }
    return voices;
}

channel_event_t *decode_next_events()
{
    static channel_event_t events[MAX_CHANNELS];
    next_event();
    for (int channel = 0; channel < module.num_channels; channel++)
    {
        size_t event_bytes = module.decode_event(pattern_line(), events + channel);
        advance_pattern_line(event_bytes);
    }
    return events;
}

void set_portamento_target(channel_event_t event, sample_t sample, voice_t *voice)
{
    voice->tone_portamento_target_period = period_for_note(event.note + sample.transpose);
}

void trigger_new_note(channel_event_t event, sample_t sample, voice_t *voice)
{
    voice->channel_playing = true;
    voice->sample_pointer = sample.sample_data;
    voice->phase_accumulator = 0.0;
    voice->arpeggiator_on = false;
    voice->current_note = event.note + sample.transpose;
    voice->period = period_for_note(voice->current_note);
    voice->tone_portamento_target_period = voice->period;
    voice->gain = get_internal_gain(sample.default_gain);
    voice->sample_repeats = sample.repeats;
    voice->repeat_length = sample.repeat_length;
    voice->sample_end = voice->sample_repeats
            ? sample.repeat_offset + sample.repeat_length
            : sample.sample_length;
}
