#include <stdio.h>
#include "player.h"
#include <math.h>
#include "sequencer.h"
#include "effects.h"
#include "lfo.h"
#include "period.h"
#include "memory/heap.h"
#include "messages.h"
#include "io/error.h"

#define DEFAULT_TICKS_PER_SECOND 50

static double fine_tuning[256] = {0};

static void calculate_fine_tuning(void);
static bool player_tick(player_t *);
static void process_commands(player_t *player);
static void process_command(player_t *player, player_command_t command);
static void process_toggle_play_command(player_t *player);
static void process_seek_command(player_t *player, seek_command_t data);
static void process_midi_note_on_command(player_t *player, midi_note_on_command_t data);
static void process_keyboard_note_on_command(player_t *player, keyboard_note_on_command_t data);
static void process_note_off_command(const player_t *player, note_off_command_t data);
static void process_toggle_loop_command(player_t *player);
static void process_set_master_gain_command(player_t *player, master_gain_command_t data);
static void process_track_mute_state_changed_command(const player_t *player, track_mute_command_t data);
static void set_current_frame(player_t *, bool);
static void reset_voice_loops(const player_t *);
static void player_step(player_t *player);
static voice_t *initialise_voices(const player_t *);
static scheduled_note_t *initialise_note_schedulers(const player_t *);
static event_t *get_events(const player_t *);
static void play_scheduled_notes(const player_t *);
static void note_on(int, const instrument_t *, player_sample_t *, uint8_t, voice_t *);
static void clear_scheduled_notes(const player_t *);
static void note_off(voice_t *voice);
static bool audio_consume(player_t *);
static void update_voices(player_t *player);
static void on_new_event(player_t *, const event_t *, voice_t *, scheduled_note_t *);
static void player_start(player_t *);
static void player_stop(player_t *);
static void player_seek(player_t *, int, int);
static player_event_t create_user_midi_event(int note);
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
    player->current_bpm = module->initial_bpm;
    const tempo_t initial_tempo = module_get_initial_tempo(module);
    player->tick_scheduler = tick_scheduler_create(initial_tempo, audio_api.info.sample_rate);
    player->sequence = initialise_sequence(module, audio_api.info.bouncing);
    player->bouncing = audio_api.info.bouncing;
    player->command_queue = command_queue_init();
    if (player->command_queue == NULL)
        goto init_failed;
    player->voices = initialise_voices(player);
    if (player->voices == NULL)
        goto init_failed;
    player->scheduled_notes = initialise_note_schedulers(player);
    if (player->scheduled_notes == NULL)
        goto init_failed;
    const bool audio_init_result = initialise_audio(&player->audio_out, audio_api, module->num_tracks, player->master_gain, module->volume_mapping_type);
    if (!audio_init_result)
    {
        error_with_detail(AUDIO_INIT_FAILED, get_error_message());
        goto init_failed;
    }
    tick_scheduler_restart(&player->tick_scheduler);
    set_current_frame(player, true);
    calculate_fine_tuning();
    player_update_samples(player);
    reset_voice_loops(player);
    lfo_init_waveforms();
    return player;

init_failed:
    player_destroy(player);
    return NULL;
}

void player_update_samples(player_t *player)
{
    const module_t *module = player->module;
    for (int i = 0; i < 256; i++)
    {
        const instrument_t instrument = module->instruments[i];
        if (!instrument.assigned) continue;
        const sample_t sample = module->samples[instrument.sample_index];
        const double ft = fine_tuning[sample.finetune + 128];
        const float base_period = (float) period_for_note(sample.base_note, ft);
        const float phase_increment_per_period = sample.sample_rate * base_period / (float) player->audio_out.api.info.sample_rate;
        player->samples[i].phase_increment_per_period = phase_increment_per_period;
        player->samples[i].fine_tuning = fine_tuning[128 + sample.finetune];
        player->samples[i].sample_repeats = instrument.repeats;
        player->samples[i].sample_end = sample.sample_length;
        player->samples[i].repeat_length = instrument.repeat_length;
        player->samples[i].sample_pointer = sample.sample_data;
    }
}

void player_set_sample_finetune(player_t *player, const int instrument_no, const int8_t finetune)
{
    if (instrument_no <= 0) return;
    player->samples[instrument_no - 1].fine_tuning = fine_tuning[128 + finetune];
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
    player->running = false;
    player->playing = false;
    destroy_audio_resources(&player->audio_out);
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

void player_sequence_changed(player_t *player, const module_t *module)
{
    player->sequence = reinitialise_sequence(module, &player->sequence, false);
}

player_restore_state_t player_get_restore_state(const player_t *player)
{
    return (player_restore_state_t) {
        .sequence = player->sequence,
        .tick_scheduler = player->tick_scheduler,
    };
}

void player_restore_state(player_t *player, const player_restore_state_t state)
{
    player->sequence = state.sequence;
    player->tick_scheduler = state.tick_scheduler;
}

void player_destroy(player_t *player)
{
    command_queue_destroy(player->command_queue);
    deallocate(PLAYER, player->voices);
    deallocate(PLAYER, player->scheduled_notes);
    deallocate(PLAYER, player);
}

void player_get_and_reset_peaks(player_t *player, float *peak_l, float *peak_r)
{
    const unsigned l = atomic_exchange_explicit(&player->audio_out.peak_l, 0, memory_order_relaxed);
    const unsigned r = atomic_exchange_explicit(&player->audio_out.peak_r, 0, memory_order_relaxed);
    *peak_l = (float) l / 65535.0f;
    *peak_r = (float) r / 65535.0f;
}

void player_set_bpm(player_t *player, const uint8_t beats_per_minute)
{
    player->current_bpm = beats_per_minute;
    if (beats_per_minute > 0)
    {
        const tempo_t tempo = player->module->tempo_lookup[beats_per_minute];
        tick_scheduler_set_tempo(&player->tick_scheduler, tempo);
    }
}

static void calculate_fine_tuning(void)
{
    for (int finetune = -128; finetune <= 127; finetune++)
        fine_tuning[finetune + 128] = pow(2.0, -(double) finetune / (16.0 * 96.0));
}

static bool player_tick(player_t *player)
{
    process_commands(player);
    if (player->bouncing && player->sequence.song_ended)
    {
        player_shutdown(player);
        return true;
    }
    tick_scheduler_t *tick_scheduler = &player->tick_scheduler;
    tick_scheduler_accumulate(&tick_scheduler->audio_accumulator);
    const bool healthy = audio_consume(player);
    if (healthy && player->playing)
    {
        player_step(player);
        update_voices(player);
        play_scheduled_notes(player);
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
            process_seek_command(player, command.data.seek);
            break;
        case MIDI_NOTE_ON:
            process_midi_note_on_command(player, command.data.midi_note_on);
            break;
        case KEYBOARD_NOTE_ON:
            process_keyboard_note_on_command(player, command.data.keyboard_note_on);
            break;
        case MIDI_NOTE_OFF:
            /* fall through */
        case KEYBOARD_NOTE_OFF:
            process_note_off_command(player, command.data.note_off);
            break;
        case TOGGLE_LOOP:
            process_toggle_loop_command(player);
            break;
        case SET_MASTER_GAIN:
            process_set_master_gain_command(player, command.data.master_gain);
            break;
        case TRACK_MUTE_STATE_CHANGED:
            process_track_mute_state_changed_command(player, command.data.track_mute);
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

static void process_seek_command(player_t *player, const seek_command_t data)
{
    player_seek(player, data.new_sequence_pos, data.new_pattern_pos);
}

static void process_midi_note_on_command(player_t *player, const midi_note_on_command_t data)
{
    event_queue_add(player->player_event_queue, create_user_midi_event(data.note));
    voice_t *voice = player->voices + data.track;
    const instrument_t instrument = player->module->instruments[data.instrument_no];
    if (!instrument.assigned)
    {
        note_off(voice);
        return;
    }
    player_sample_t *sample = &player->samples[data.instrument_no];
    note_on(data.note, &instrument, sample, 0, voice);
    voice->volume = instrument.default_volume;
}

static void process_keyboard_note_on_command(player_t *player, const keyboard_note_on_command_t data)
{
    voice_t *voice = player->voices + data.track;
    const instrument_t instrument = player->module->instruments[data.instrument_no];
    if (!instrument.assigned)
    {
        note_off(voice);
        return;
    }
    player_sample_t *sample = &player->samples[data.instrument_no];
    note_on(data.note, &instrument, sample, 0, voice);
    voice->volume = instrument.default_volume;
}

static void process_note_off_command(const player_t *player, const note_off_command_t data)
{
    voice_t *voice = player->voices + data.track;
    const instrument_t instrument = player->module->instruments[data.instrument_no];
    if (instrument.assigned && instrument.repeats)
        note_off(voice);
}

static void process_toggle_loop_command(player_t *player)
{
    if (player->sequence.looping_state.looping)
        clear_pattern_loop(&player->sequence);
    else
        set_pattern_loop(&player->sequence);
}

static void process_set_master_gain_command(player_t *player, const master_gain_command_t data)
{
    player->audio_out.master_gain = data.master_gain;
    player->module->master_gain = data.master_gain;
}

static void process_track_mute_state_changed_command(const player_t *player, const track_mute_command_t data)
{
    const int track = data.track;
    if (track < 0 || track >= player->module->num_tracks)
        return;
    const bool new_state = player->module->tracks[track].muted;
    voice_t *voice = player->voices + track;
    voice->muted = new_state;
}

static voice_t *initialise_voices(const player_t *player)
{
    voice_t *voices = allocate_array(PLAYER, player->module->num_tracks, sizeof(voice_t));
    if (voices == NULL)
        return NULL;
    for (int channel = 0; channel < player->module->num_tracks; channel++)
    {
        voices[channel].channel_playing = false;
        voices[channel].muted = player->module->tracks[channel].muted;
        voices[channel].arpeggiator_on = false;
        voices[channel].panning = player->module->tracks[channel].panning - 1;
        voices[channel].volume = INTERNAL_GAIN_MAX;
        voices[channel].period_modulation = 0;
        voices[channel].vibrato.enabled = false;
        voices[channel].vibrato.retrigger = true;
        voices[channel].vibrato.waveform = PT_WAVEFORM_SINE;
        voices[channel].vibrato.phase = 0;
        voices[channel].tremolo.enabled = false;
        voices[channel].tremolo.retrigger = true;
        voices[channel].tremolo.waveform = PT_WAVEFORM_SINE;
        voices[channel].tremolo.phase = 0;
    }
    return voices;
}

static scheduled_note_t *initialise_note_schedulers(const player_t *player)
{
    scheduled_note_t *note_schedulers = allocate_array(PLAYER, player->module->num_tracks, sizeof(scheduled_note_t));
    if (note_schedulers == NULL)
    {
        return NULL;
    }
    for (int channel = 0; channel < player->module->num_tracks; channel++)
    {
        note_schedulers[channel].scheduled = false;
    }
    return note_schedulers;
}

static void set_current_frame(player_t *player, const bool row_advanced)
{
    player->current_frame.events = get_events(player);
    player->current_frame.row_advanced = row_advanced;
    player->current_frame.sequence_pos = player->sequence.sequence_pos;
    player->current_frame.pattern_pos = player->sequence.pattern_index;
    player->current_frame.num_tracks = player->module->num_tracks;
}

static void reset_voice_loops(const player_t *player)
{
    for (int track = 0; track < player->module->num_tracks; track++)
    {
        voice_t *voice = player->voices + track;
        voice->loop_state.start = 0;
        voice->loop_state.counter = 0;
        voice->loop_state.looping = false;
    }
}

static void player_step(player_t *player)
{
    bool row_advanced = false;
    bool sequence_advanced = false;
    if (tick_scheduler_is_new_event(&player->tick_scheduler.event_scheduler))
    {
        row_advanced = true;
        pattern_step(&player->sequence, &sequence_advanced);
    }
    else if (tick_scheduler_just_started(&player->tick_scheduler.event_scheduler))
    {
        row_advanced = true;
    }
    set_current_frame(player, row_advanced);
    if (row_advanced) clear_scheduled_notes(player);
    if (sequence_advanced) reset_voice_loops(player);
}

static event_t *get_events(const player_t *player)
{
    const int track_capacity = (int) player->module->track_capacity;
    const sequence_t *sequence = &player->sequence;
    const int pattern_no = sequence->sequence[sequence->sequence_pos];
    const pattern_t pattern = player->module->patterns[pattern_no];
    return pattern.events + sequence->pattern_index * track_capacity;
}

static bool audio_consume(player_t *player)
{
    audio_accumulator_t *audio_accumulator = &player->tick_scheduler.audio_accumulator;
    const int samples_to_write = tick_scheduler_samples_to_write(audio_accumulator);
    const bool result = write_audio_data(&player->audio_out, player->voices, samples_to_write);
    if (result)
        tick_scheduler_consume_samples(audio_accumulator);
    else
        player->error_message = get_error_message();
    return result;
}

static void update_voices(player_t *player)
{
    const frame_t *current_frame = &player->current_frame;
    for (int track = 0; track < player->module->num_tracks; track++)
    {
        const event_t *event = current_frame->events + track;
        voice_t *voice = player->voices + track;
        if (current_frame->row_advanced)
            on_new_event(player, event, voice, player->scheduled_notes + track);
        else
            handle_effects_off_event(event, voice, player);
    }
}

static void on_new_event(player_t *player, const event_t *event, voice_t *voice, scheduled_note_t *scheduler)
{
    const bool instrument_changed = event->instrument_no && event->instrument_no != voice->instrument_no;
    if (event->instrument_no)
    {
        voice->instrument_no = event->instrument_no;
    }
    handle_effects_before_note(event, voice, player);
    if (event->note)
    {
        const int note = event->note - 1;
        const instrument_t *instrument = &player->module->instruments[voice->instrument_no - 1];
        if (!instrument->assigned)
        {
            voice->channel_playing = false;
        }
        else
        {
            player_sample_t *sample = &player->samples[voice->instrument_no - 1];
            if (!instrument_changed && portamento(event))
            {
                voice->tone_portamento_target_period = period_for_note(note + instrument->transpose, sample->fine_tuning);
            }
            else
            {
                *scheduler = (scheduled_note_t) {
                    .scheduled = true,
                    .delay = get_note_delay(event),
                    .slice = get_sample_slice(event),
                    .instrument = instrument,
                    .sample = sample,
                    .note = note,
                    .voice = voice,
                };
                if (event->instrument_no)
                {
                    voice->volume = instrument->default_volume;
                }
            }
        }
    }
    else if (event->instrument_no)
    {
        const instrument_t *instrument = &player->module->instruments[voice->instrument_no - 1];
        if (instrument->assigned)
        {
            voice->volume = instrument->default_volume;
        }
    }
    handle_effects_on_event(event, voice, player);
}

static void play_scheduled_notes(const player_t *player)
{
    for (int track = 0; track < player->module->num_tracks; track++)
    {
        scheduled_note_t *s = player->scheduled_notes + track;
        if (s->scheduled && s->delay == player->tick_scheduler.event_scheduler.ticks)
        {
            note_on(s->note, s->instrument, s->sample, s->slice, s->voice);
            s->scheduled = false;
        }
    }
}

static void note_on(const int note, const instrument_t *instrument, player_sample_t *sample, const uint8_t slice, voice_t *voice)
{
    voice->channel_playing = true;
    voice->sample = sample;
    voice->arpeggiator_on = false;
    voice->current_note = note + instrument->transpose;
    voice->period = period_for_note(voice->current_note, voice->sample->fine_tuning);
    voice->tone_portamento_target_period = voice->period;
    const sample_slice_t sample_slice = instrument->sample_slices[slice];
    if (sample_slice.length == 0 || sample_slice.offset >= (uint32_t) sample->sample_end)
        voice->phase_accumulator = 0.0f;
    else
    {
        voice->phase_accumulator = (float) sample_slice.offset;
        if (sample_slice.offset + sample_slice.length < (uint32_t) voice->sample->sample_end)
            voice->sample->sample_end = (int) (sample_slice.offset + sample_slice.length);
    }
}

static void clear_scheduled_notes(const player_t *player)
{
    for (int track = 0; track < player->module->num_tracks; track++)
    {
        player->scheduled_notes[track].scheduled = false;
    }
}

static void note_off(voice_t *voice)
{
    voice->channel_playing = false;
}

static void player_stop(player_t *player)
{
    player->playing = false;
    for (int channel = 0; channel < player->module->num_tracks; channel++)
        player->voices[channel].channel_playing = false;
}

static void player_start(player_t *player)
{
    player->playing = true;
    player->sequence.song_ended = false;
    tick_scheduler_restart(&player->tick_scheduler);
}

static void player_seek(player_t *player, const int new_sequence_pos, const int new_pattern_pos)
{
    sequence_seek(&player->sequence, new_sequence_pos, new_pattern_pos);
    set_current_frame(player, true);
}

static player_event_t create_user_midi_event(const int note)
{
    return (player_event_t) {
        .type = USER_MIDI_NOTE_ON,
        .data = (player_event_data_t) {
            .midi_note = (player_midi_note_event_t) {
                .midi_note = note,
            }
        }
    };
}

static player_event_t create_error_event(const char *error_message)
{
    player_event_t event = {
        .type = PLAYER_ERROR,
    };
    snprintf(event.data.player_error.error_message, ERROR_MESSAGE_MAX_LENGTH, "%s", error_message);
    return event;
}
