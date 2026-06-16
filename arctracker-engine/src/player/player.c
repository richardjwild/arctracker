#include <stdio.h>
#include "player.h"
#include "sequencer.h"
#include "effects.h"
#include "period.h"
#include "memory/heap.h"
#include "messages.h"
#include "io/error.h"

static bool player_tick(player_t *);
static void process_commands(player_t *player);
static void process_command(player_t *player, player_command_t command);
static void process_toggle_play_command(player_t *player);
static void process_seek_command(player_t *player, player_command_t command);
static void process_note_on_command(const player_t *player, player_command_t command, bool from_midi);
static void process_toggle_loop_command(player_t *player);
static void set_current_frame(player_t *, bool);
static void player_step(player_t *player);
static voice_t *initialise_voices(const player_t *);
static event_t *get_events(const player_t *);
static void note_on(int, const instrument_t *, const sample_t *, voice_t *);
static void note_off(voice_t *voice);
static bool audio_consume(player_t *);
static void update_voices(player_t *player);
static void on_new_event(player_t *, const event_t *, voice_t *);
static void player_start(player_t *);
static void player_stop(player_t *);
static void player_seek(player_t *, int, int);
static player_event_t create_user_midi_event(int note);
static player_event_t create_audio_overflowed_event(void);
static player_event_t create_error_event(const char *);

player_t *player_create(module_t *module, const audio_api_t audio_api, player_event_queue_t *player_event_queue)
{
    player_t *player = allocate_array(PLAYER, 1, sizeof(player_t));
    if (player == NULL)
        return NULL;
    player->module = module;
    player->playing = false;
    player->player_event_queue = player_event_queue;
    player->module = module;
    player->master_gain = module->master_gain;
    player->tick_scheduler = tick_scheduler_create(module->initial_speed, audio_api.sample_rate);
    player->sequence = initialise_sequence(module, audio_api.bouncing);
    player->bouncing = audio_api.bouncing;
    player->command_queue = command_queue_init();
    if (player->command_queue == NULL)
        goto init_failed;
    player->voices = initialise_voices(player);
    if (player->voices == NULL)
        goto init_failed;
    const audio_out_result_t audio_init_result = initialise_audio(&player->audio_out, audio_api, module->num_channels, player->master_gain);
    if (!audio_init_result.success)
    {
        error_with_detail(AUDIO_INIT_FAILED, audio_init_result.error_message);
        goto init_failed;
    }
    tick_scheduler_restart(&player->tick_scheduler);
    set_current_frame(player, true);
    return player;

init_failed:
    player_destroy(player);
    return NULL;
}

bool player_run(player_t *player)
{
    player->running = true;
    if (player->bouncing)
    {
        // Normally the player waits to be started by the user,
        // but when bouncing audio, it must start immediately.
        player_start(player);
    }
    bool healthy = true;
    while (healthy && player->running)
    {
        // This is the player main loop.
        healthy = player_tick(player);
    }
    if (healthy)
        send_remaining_audio(&player->audio_out);
    else
        event_queue_add(player->player_event_queue, create_error_event(player->error_message));
    destroy_audio_resources(&player->audio_out);
    deallocate(PLAYER, player->voices);
    player->running = false;
    player->playing = false;
    return healthy;
}

bool player_queue_command(const player_t *player, const player_command_t command)
{
    return command_queue_add(player->command_queue, command);
}

void player_shutdown(player_t *player)
{
    player->running = false;
}

void player_sequence_changed(player_t *player, module_t *module)
{
    player->sequence = reinitialise_sequence(module, &player->sequence, false);
}

void player_destroy(player_t *player)
{
    command_queue_destroy(player->command_queue);
    deallocate(PLAYER, player);
}

static bool player_tick(player_t *player)
{
    process_commands(player);
    if (player->bouncing && player->sequence.looped)
    {
        player_shutdown(player);
        return true;
    }
    tick_scheduler_t *tick_scheduler = &player->tick_scheduler;
    tick_scheduler_accumulate(&tick_scheduler->audio_accumulator);
    const bool healthy = audio_consume(player);
    if (healthy && player->playing)
    {
        update_voices(player);
        player_step(player);
        tick_scheduler_advance_tick(&tick_scheduler->event_scheduler);
    }
    return healthy;
}

static void process_commands(player_t *player)
{
    player_command_t command;
    while (command_queue_read(player->command_queue, &command))
        process_command(player, command);
}

static void process_command(player_t *player, const player_command_t command)
{
    switch (command.cmd_type)
    {
        case TOGGLE_PLAY:
            process_toggle_play_command(player);
            break;
        case SEEK:
            process_seek_command(player, command);
            break;
        case MIDI_NOTE_ON:
            process_note_on_command(player, command, true);
            break;
        case KEYBOARD_NOTE_ON:
            process_note_on_command(player, command, false);
            break;
        case TOGGLE_LOOP:
            process_toggle_loop_command(player);
            break;
        default:
            break;
    }
}

static void process_toggle_play_command(player_t *player)
{
    if (player->playing)
        player_stop(player);
    else
        player_start(player);
}

static void process_seek_command(player_t *player, const player_command_t command)
{
    player_seek(player, command.new_sequence_pos, command.new_pattern_pos);
}

static void process_note_on_command(const player_t *player, const player_command_t command, const bool from_midi)
{
    if (from_midi)
    {
        event_queue_add(player->player_event_queue, create_user_midi_event(command.note));
    }
    voice_t *voice = player->voices + command.channel_no;
    const instrument_t instrument = player->module->instruments[command.instrument_no];
    if (!instrument.assigned)
    {
        note_off(voice);
        return;
    }
    const sample_t *sample = player->module->samples + instrument.sample_index;
    note_on(command.note, &instrument, sample, voice);
}

static void process_toggle_loop_command(player_t *player)
{
    if (player->sequence.looping_state.looping)
        clear_pattern_loop(&player->sequence);
    else
        set_pattern_loop(&player->sequence);
}

static voice_t *initialise_voices(const player_t *player)
{
    voice_t *voices = allocate_array(PLAYER, player->module->num_channels, sizeof(voice_t));
    if (voices == NULL)
        return NULL;
    for (int channel = 0; channel < player->module->num_channels; channel++)
    {
        voices[channel].channel_playing = false;
        voices[channel].arpeggiator_on = false;
        voices[channel].panning = player->module->initial_panning[channel] - 1;
        voices[channel].volume = INTERNAL_GAIN_MAX;
    }
    return voices;
}

static void set_current_frame(player_t *player, const bool row_advanced)
{
    player->current_frame.events = get_events(player);
    player->current_frame.row_advanced = row_advanced;
    player->current_frame.sequence_pos = player->sequence.sequence_pos;
    player->current_frame.pattern_pos = player->sequence.pattern_index;
    player->current_frame.num_channels = player->module->num_channels;
}

static void player_step(player_t *player)
{
    bool row_advanced = false;
    if (tick_scheduler_is_new_event(&player->tick_scheduler.event_scheduler))
    {
        sequence_advance(&player->sequence);
        row_advanced = true;
    }
    set_current_frame(player, row_advanced);
}

static event_t *get_events(const player_t *player)
{
    const int num_channels = player->module->num_channels;
    const sequence_t *sequence = &player->sequence;
    const int pattern_no = sequence->sequence[sequence->sequence_pos];
    const pattern_t pattern = player->module->patterns[pattern_no];
    return pattern.events + sequence->pattern_index * num_channels;
}

static bool audio_consume(player_t *player)
{
    audio_accumulator_t *audio_accumulator = &player->tick_scheduler.audio_accumulator;
    const int samples_to_write = tick_scheduler_samples_to_write(audio_accumulator);
    const audio_out_result_t result = write_audio_data(&player->audio_out, player->voices, samples_to_write);
    if (result.success)
    {
        tick_scheduler_consume_samples(audio_accumulator);
        if (result.overflowed)
            event_queue_add(player->player_event_queue, create_audio_overflowed_event());
    }
    else
        player->error_message = result.error_message;
    return result.success;
}

static void update_voices(player_t *player)
{
    const frame_t *current_frame = &player->current_frame;
    for (int channel = 0; channel < player->module->num_channels; channel++)
    {
        const event_t *event = current_frame->events + channel;
        voice_t *voice = player->voices + channel;
        if (current_frame->row_advanced)
            on_new_event(player, event, voice);
        else
            handle_effects_off_event(event, voice);
    }
}

static void on_new_event(player_t *player, const event_t *event, voice_t *voice)
{
    const instrument_t instrument = player->module->instruments[event->instrument_no - 1];
    if (event->note && instrument.assigned)
    {
        const sample_t *sample = player->module->samples + instrument.sample_index;
        if (portamento(event))
            voice->tone_portamento_target_period = period_for_note(event->note + instrument.transpose);
        else
            note_on(event->note, &instrument, sample, voice);
    }
    else if (event->note && !instrument.assigned)
    {
        voice->channel_playing = false;
    }
    else if (event->instrument_no && instrument.assigned)
    {
        voice->volume = instrument.default_volume;
    }
    reset_arpeggiator(voice);
    handle_effects_on_event(event, voice, player);
}

static void note_on(const int note, const instrument_t *instrument, const sample_t *sample, voice_t *voice)
{
    voice->channel_playing = true;
    voice->sample_pointer = sample->sample_data;
    voice->phase_accumulator = 0.0f;
    voice->arpeggiator_on = false;
    voice->current_note = note + instrument->transpose;
    voice->period = period_for_note(voice->current_note);
    voice->tone_portamento_target_period = voice->period;
    voice->volume = instrument->default_volume;
    voice->sample_repeats = instrument->repeats;
    voice->repeat_length = instrument->repeat_length;
    voice->sample_end = voice->sample_repeats
            ? instrument->repeat_offset + instrument->repeat_length
            : sample->sample_length;
}

static void note_off(voice_t *voice)
{
    voice->channel_playing = false;
}

static void player_stop(player_t *player)
{
    player->playing = false;
    for (int channel = 0; channel < player->module->num_channels; channel++)
        player->voices[channel].channel_playing = false;
}

static void player_start(player_t *player)
{
    player->playing = true;
    player->sequence.looped = false;
    tick_scheduler_restart(&player->tick_scheduler);
}

static void player_seek(player_t *player, const int new_sequence_pos, const int new_pattern_pos)
{
    sequence_seek(&player->sequence, new_sequence_pos, new_pattern_pos);
    set_current_frame(player, true);
}

static player_event_t create_user_midi_event(const int note)
{
    player_event_t event = {0};
    event.type = USER_MIDI_NOTE_ON;
    event.midi_note = note;
    return event;
}

static player_event_t create_audio_overflowed_event(void)
{
    return (player_event_t) {
        .type = AUDIO_OVERFLOWED
    };
}

static player_event_t create_error_event(const char *error_message)
{
    player_event_t event = {0};
    event.type = PLAYER_ERROR;
    snprintf(event.error_message, ERROR_MESSAGE_MAX_LENGTH, "%s", error_message);
    return event;
}
