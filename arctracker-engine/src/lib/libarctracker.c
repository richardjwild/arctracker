#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "libarctracker.h"
#include "arctracker.h"
#include "messages.h"
#include "player/player.h"
#include "audio_api/api.h"
#include "audio_api/api_portaudio.h"
#include "audio_api/api_wav.h"
#include "io/error.h"
#include "loader/loader.h"
#include "memory/heap.h"
#include "memory/bits.h"
#include "editor/editor.h"
#include "loader/format_arctracker.h"
#include "midi/midi.h"

#define SUCCESS (api_result_t) {\
    .success = true,\
    .error_message = {0}\
}
#define MAX_PATTERN_LENGTH 1000

static void *run_player(void *);
static void get_player_transport_state(const audio_subsystem_t *playback, module_t *module, ui_transport_state_t *transport_state);
static void copy_pattern_line(ui_pattern_event_t *line_buffer, event_t *events, int requested_tracks, int module_tracks);
static void to_ui_event(ui_pattern_event_t *event_buffer, event_t *event);
static void to_internal_event(event_t *event_buffer, ui_pattern_event_t *event);
static void *run_export(void *);
static void remove_module(arctracker_t *);
static api_result_t failure(char *);
static void count_resources(resource_group_t, char *);

arctracker_init_result_t arctracker_init(void)
{
    arctracker_t *arctracker = arctracker_create();
    midi_subsystem_t *midi = midi_initialise(arctracker);
    return (arctracker_init_result_t) {
        .arctracker_handle = arctracker,
        .midi_handle = midi,
    };
}

arctracker_t *arctracker_create(void)
{
    arctracker_t *arctracker = allocate_array(MAIN, 1, sizeof(arctracker_t));
    if (arctracker == NULL)
        return NULL;
    arctracker->module = NULL;
    arctracker->playback.player = NULL;
    arctracker->export.player = NULL;
    arctracker->playback.thread_active = false;
    arctracker->export.thread_active = false;
    arctracker->export_state = IDLE,
    arctracker->playback.event_queue = event_queue_init();
    arctracker->export.event_queue = event_queue_init();
    if (arctracker->playback.event_queue == NULL || arctracker->export.event_queue == NULL)
    {
        arctracker_destroy(arctracker);
        return NULL;
    }
    arctracker->playback.initialised = start_portaudio();
    if (!arctracker->playback.initialised)
    {
        fprintf(stderr, "%s\n", get_error_message());
        clear_error_state();
    }
    return arctracker;
}

int arctracker_get_available_output_count(arctracker_t *arctracker)
{
    if (arctracker == NULL || !arctracker->playback.initialised)
        return 0;
    return get_available_output_count();
}

api_result_t arctracker_get_available_outputs(arctracker_t *arctracker, ui_audio_device_info_t *output_devices, int requested_outputs)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (!arctracker->playback.initialised)
        return failure(AUDIO_SYSTEM_NOT_INITIALISED);
    audio_device_info_t *devices = allocate_array(MAIN, sizeof(audio_device_info_t), requested_outputs);
    api_result_t result;
    if (!get_available_outputs(devices, requested_outputs))
        result = failure(FAILED_TO_GET_AVAILABLE_OUTPUTS);
    else
    {
        for (int i = 0; i < requested_outputs; i++)
        {
            output_devices[i].device_index = devices[i].device_index;
            snprintf(output_devices[i].name, sizeof(output_devices[i].name), "%s", devices[i].name);
            snprintf(output_devices[i].host_api_name, sizeof(output_devices[i].name), "%s", devices[i].host_api_name);
        }
        result = SUCCESS;
    }
    deallocate(MAIN, devices);
    return result;
}

static api_result_t use_output(arctracker_t *arctracker, audio_api_t audio_api)
{
    arctracker->playback_audio_api = audio_api;
    if (arctracker->playback.thread_active)
    {
        const api_result_t result = arctracker_player_shutdown(arctracker);
        if (!result.success)
            return result;
    }
    return arctracker_player_start(arctracker);
}

api_result_t arctracker_use_output(arctracker_t *arctracker, const int device_index, const char *name, const char *host_api_name)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    const audio_api_t audio_api = get_output(device_index, name, host_api_name);
    return use_output(arctracker, audio_api);
}

api_result_t arctracker_use_default_output(arctracker_t *arctracker)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    const audio_api_t audio_api = get_default_output();
    return use_output(arctracker, audio_api);
}

int arctracker_get_available_midi_count(midi_subsystem_t *midi)
{
    if (midi == NULL)
        return 0;
    unsigned int count = 0;
    midi_get_device_count(midi, &count);
    return (int) count;
}

api_result_t arctracker_get_available_midi_devices(midi_subsystem_t *midi, ui_midi_device_info_t *devices_out, int requested_count)
{
    if (midi == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    midi_device_info_t *devices = allocate_array(MAIN, sizeof(midi_device_info_t), requested_count);
    if (devices == NULL)
        return failure(MEMORY_ALLOCATION_FAILED);
    api_result_t result;
    if (!midi_get_devices(midi, devices, requested_count))
        result = failure(FAILED_TO_GET_AVAILABLE_MIDI_DEVICES);
    else
    {
        for (int i = 0; i < requested_count; i++)
            snprintf(devices_out[i].name, sizeof(devices_out[i].name), "%s", devices[i].name);
        result = SUCCESS;
    }
    deallocate(MAIN, devices);
    return result;
}

api_result_t arctracker_use_midi_device(midi_subsystem_t *midi, const char *name)
{
    if (midi == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (!midi_use_device(midi, name))
        return failure(FAILED_TO_USE_MIDI_DEVICE);
    return SUCCESS;
}

void arctracker_midi_set_playback_channel(midi_subsystem_t *midi, const int channel)
{
    if (midi != NULL)
        midi_set_playback_channel(midi, channel);
}

void arctracker_midi_set_playback_instrument(midi_subsystem_t *midi, const uint8_t instrument)
{
    if (midi != NULL)
        midi_set_playback_instrument(midi, instrument);
}

void arctracker_midi_keyboard_note_on(const midi_subsystem_t *midi, const int note)
{
    if (midi != NULL)
        keyboard_note_on(midi, note);
}

api_result_t arctracker_get_current_module(arctracker_t *arctracker, ui_module_info_t *module_info)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    memset(module_info, 0, sizeof(ui_module_info_t));
    if (arctracker->module != NULL)
        module_get_info(arctracker->module, module_info);
    return SUCCESS;
}

api_result_t arctracker_module_load(arctracker_t *arctracker, char *mod_filename, ui_module_info_t *module_info)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    const load_module_result_t load_result = load_module(mod_filename);
    if (!load_result.file_read)
        return failure(FILE_OPEN_FAILED);
    if (!load_result.recognised_format)
        return failure(MODULE_TYPE_UNRECOGNISED);
    if (!load_result.module_loaded)
        return failure(MODULE_LOAD_FAILED);
    if (arctracker->playback.thread_active)
        arctracker_player_shutdown(arctracker);
    if (arctracker->module != NULL)
        remove_module(arctracker);
    arctracker->module = load_result.module;
    module_get_info(arctracker->module, module_info);
    return SUCCESS;
}

api_result_t arctracker_module_save(const arctracker_t *arctracker, const char *mod_filename, const int format)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (mod_filename == NULL || strlen(mod_filename) == 0)
        return failure(BAD_FILENAME);
    if (format != FORMAT_ARCTRACKER)
        return failure(BAD_FORMAT);
    if (!save_module(arctracker->module, mod_filename, arctracker_format()))
        return failure(MODULE_SAVE_FAILED);
    return SUCCESS;
}

api_result_t arctracker_get_instrument_info(arctracker_t *arctracker, uint8_t slot, ui_instrument_info_t *instrument_info)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (instrument_info == NULL)
        return failure(BAD_BUFFER);
    module_get_instrument_info(arctracker->module, slot, instrument_info);
    return SUCCESS;
}

api_result_t arctracker_module_create(arctracker_t *arctracker, const new_module_params_t params, ui_module_info_t *module_info)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    module_t *module = module_create(params.num_tracks, 1, 1, 0);
    if (module == NULL || !module_init(module, params.default_pattern_length, params.lines_per_beat, params.beats_per_minute))
        return failure(MODULE_CREATE_FAILED);
    strncpy(module->name, NEW_MODULE_TITLE, MAX_LEN_TUNENAME);
    strncpy(module->author, params.author, MAX_LEN_AUTHOR);
    if (arctracker->playback.thread_active)
        arctracker_player_shutdown(arctracker);
    if (arctracker->module != NULL)
        remove_module(arctracker);
    arctracker->module = module;
    module_get_info(arctracker->module, module_info);
    return SUCCESS;
}

api_result_t arctracker_get_pattern_lengths(arctracker_t *arctracker, int *pattern_lengths, int num_patterns)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (pattern_lengths == NULL)
        return failure(BAD_PATTERN_LENGTH_BUFFER);
    const module_t *module = arctracker->module;
    if (num_patterns != module->num_patterns)
        return failure(INVALID_PATTERN_COUNT);
    for (int pattern = 0; pattern < num_patterns; pattern++)
        pattern_lengths[pattern] = module->patterns[pattern].num_lines;
    return SUCCESS;
}

api_result_t arctracker_player_start(arctracker_t *arctracker)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (!arctracker->playback.initialised)
        return failure(AUDIO_SYSTEM_NOT_INITIALISED);
    if (arctracker->playback.thread_active)
        return failure(PLAYER_ALREADY_RUNNING);
    if (arctracker->playback_audio_api.init == NULL)
        return failure(AUDIO_OUTPUT_NOT_CONFIGURED);
    if (has_error())
    {
        return failure(AUDIO_INIT_FAILED);
    }
    arctracker->playback.player = player_create(arctracker->module, arctracker->playback_audio_api, arctracker->playback.event_queue);
    if (arctracker->playback.player == NULL)
        return failure(PLAYER_INIT_FAILED);
    pthread_t audio_thread;
    int err = pthread_create(&audio_thread, NULL, run_player, arctracker->playback.player);
    if (err != 0)
    {
        error(strerror(err));
        return failure(PLAYER_THREAD_CREATE_FAILED);
    }
    arctracker->playback.thread_active = true;
    arctracker->playback.audio_thread = audio_thread;
    return SUCCESS;
}

static void *run_player(void *arg)
{
    player_t *player = arg;
    player_run(player);
    return NULL;
}

bool arctracker_player_cmd(arctracker_t *arctracker, player_command_t *command)
{
    if (arctracker == NULL || !arctracker->playback.thread_active) return false;
    return player_queue_command(arctracker->playback.player, *command);
}

bool arctracker_poll_playback_event(arctracker_t *arctracker, player_event_t *event)
{
    if (arctracker == NULL) return false;
    return event_queue_read(arctracker->playback.event_queue, event);
}

bool arctracker_poll_export_event(arctracker_t *arctracker, player_event_t *event)
{
    if (arctracker == NULL) return false;
    if (!event_queue_read(arctracker->export.event_queue, event))
        return false;
    return (event->type == PLAYER_ERROR);
}

void arctracker_get_transport_state(arctracker_t *arctracker, ui_transport_state_t *transport_state)
{
    if (arctracker == NULL)
        return;
    if (arctracker->playback.thread_active)
    {
        get_player_transport_state(&arctracker->playback, arctracker->module, transport_state);
        return;
    }
    const int pattern_no = arctracker->module->sequence[0];
    transport_state->playing = false;
    transport_state->playback_available = false;
    transport_state->looping = false;
    transport_state->sequence_pos = 0;
    transport_state->pattern_index = 0;
    transport_state->pattern_no = pattern_no;
    transport_state->pattern_length = arctracker->module->patterns[pattern_no].num_lines;
    transport_state->current_bpm = 0;
}

void arctracker_get_track_state(arctracker_t *arctracker, ui_track_state_t *track_state, const int track)
{
    if (arctracker == NULL || arctracker->module == NULL)
        return;
    if (track < 0 || track >= arctracker->module->num_tracks)
        return;
    track_state->effects_displayed = arctracker->module->tracks[track].effects_displayed;
    track_state->muted = arctracker->module->tracks[track].muted;
    if (arctracker->playback.thread_active && arctracker->playback.player->running)
        track_state->panning = arctracker->playback.player->voices[track].panning;
    else
        track_state->panning = arctracker->module->tracks[track].panning;
}

void arctracker_toggle_mute_state(arctracker_t *arctracker, const int track)
{
    if (arctracker == NULL || arctracker->module == NULL)
        return;
    if (track < 0 || track >= arctracker->module->num_tracks)
        return;
    module_toggle_mute_state(arctracker->module, track);
}

void arctracker_set_effects_displayed(arctracker_t *arctracker, int track, int effects_displayed)
{
    if (arctracker == NULL || arctracker->module == NULL)
        return;
    if (track < 0 || track >= arctracker->module->num_tracks)
        return;
    if (effects_displayed < 0 || effects_displayed > 4)
        return;
    module_set_effects_displayed(arctracker->module, track, effects_displayed);
}

void arctracker_get_and_reset_peak_levels(arctracker_t *handle, ui_peak_level_t *peak_levels)
{
    if (handle == NULL || !handle->playback.thread_active)
    {
        peak_levels->left = 0.0f;
        peak_levels->right = 0.0f;
        return;
    }
    player_get_and_reset_peaks(handle->playback.player, &peak_levels->left, &peak_levels->right);
}

void arctracker_get_export_state(arctracker_t *arctracker, ui_export_state_t *export_state)
{
    if (arctracker == NULL || !arctracker->export.thread_active)
    {
        export_state->completed = false;
        export_state->percent_complete = 0;
    }
    else if (arctracker->export_state == COMPLETE)
    {
        export_state->completed = true;
        export_state->percent_complete = 100;
    }
    else
    {
        export_state->completed = false;
        export_state->percent_complete = (arctracker->export.player->sequence.sequence_pos * 100) / (arctracker->module->sequence_length);
    }
}

static void get_player_transport_state(const audio_subsystem_t *playback, module_t *module, ui_transport_state_t *transport_state)
{
    const player_t *player = playback->player;
    const sequence_t sequence = player->sequence;
    const int pattern_no = module->sequence[sequence.sequence_pos];
    transport_state->playing = player->playing;
    transport_state->playback_available = playback->thread_active;
    transport_state->current_bpm = player->current_bpm;
    transport_state->looping = player->sequence.looping_state.looping;
    transport_state->sequence_pos = sequence.sequence_pos;
    transport_state->pattern_index = sequence.pattern_index;
    transport_state->pattern_no = pattern_no;
    transport_state->pattern_length = module->patterns[pattern_no].num_lines;
}

void arctracker_get_pattern(arctracker_t *arctracker, int pattern_no, ui_pattern_event_t *pattern_buffer, int requested_lines, int requested_tracks)
{
    if (pattern_buffer == NULL || requested_lines <= 0 || requested_tracks <= 0)
        return;
    const size_t requested_events = (size_t) requested_lines * (size_t) requested_tracks;
    memset(pattern_buffer, 0, sizeof(ui_pattern_event_t) * requested_events);
    if (arctracker == NULL || arctracker->module == NULL)
        return;
    const module_t *module = arctracker->module;
    if (pattern_no < 0 || pattern_no >= module->num_patterns)
        return;
    const pattern_t pattern = module->patterns[pattern_no];
    if (pattern.events == NULL)
        return;
    event_t *events = pattern.events;
    ui_pattern_event_t *line_buffer = pattern_buffer;
    for (int line = 0; line < requested_lines; line++)
    {
        if (line < pattern.num_lines)
            copy_pattern_line(line_buffer, events, requested_tracks, module->num_tracks);
        line_buffer += requested_tracks;
        events += module->track_capacity;
    }
}

static void copy_pattern_line(ui_pattern_event_t *line_buffer, event_t *events, int requested_tracks, int module_tracks)
{
    ui_pattern_event_t *event_buffer = line_buffer;
    event_t *event = events;
    for (int track = 0; track < requested_tracks; track++)
    {
        if (track < module_tracks)
            to_ui_event(event_buffer, event);
        event_buffer++;
        event++;
    }
}

static void to_ui_event(ui_pattern_event_t *event_buffer, event_t *event)
{
    event_buffer->note = event->note;
    event_buffer->sample_no = event->instrument_no;
    for (int effect_no = 0; effect_no < 4; effect_no++)
    {
        const effect_t effect = event->effects[effect_no];
        event_buffer->effects[effect_no].effect_code = effect.command;
        event_buffer->effects[effect_no].effect_data[0] = HIGH_NYBBLE(effect.data);
        event_buffer->effects[effect_no].effect_data[1] = LOW_NYBBLE(effect.data);
    }
}

static void to_internal_event(event_t *event_buffer, ui_pattern_event_t *event)
{
    memset(event_buffer, 0, sizeof(event_t));
    event_buffer->note = event->note;
    event_buffer->instrument_no = event->sample_no;
    for (int effect_no = 0; effect_no < 4; effect_no++)
    {
        const ui_effect_t effect = event->effects[effect_no];
        event_buffer->effects[effect_no].command = (unsigned) effect.effect_code;
        // Reading left to right gives us most significant nybble first.
        event_buffer->effects[effect_no].data =
                ((effect.effect_data[0] & 0xf) << 4)
                + (effect.effect_data[1] & 0xf);
    }
}

api_result_t arctracker_export_audio(arctracker_t *arctracker, char *output_filename)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (arctracker->export.thread_active)
        return failure(PLAYER_ALREADY_RUNNING);
    audio_api_t audio_api = initialise_wav(output_filename);
    arctracker->export.player = player_create(arctracker->module, audio_api, arctracker->export.event_queue);
    if (arctracker->export.player == NULL)
        return failure(EXPORT_INIT_FAILED);
    pthread_t export_thread;
    int err = pthread_create(&export_thread, NULL, run_export, arctracker);
    if (err != 0)
    {
        error(strerror(err));
        return failure(EXPORT_THREAD_CREATE_FAILED);
    }
    arctracker->export.thread_active = true;
    arctracker->export.audio_thread = export_thread;
    arctracker->export_state = RUNNING;
    return SUCCESS;
}

api_result_t arctracker_export_sample(arctracker_t *arctracker, int instrument_no, char *output_filename)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (instrument_no < 0 || instrument_no >= NUM_INSTRUMENT_SLOTS)
        return failure(INVALID_INSTRUMENT_INDEX);
    const instrument_t instrument = arctracker->module->instruments[instrument_no];
    if (!instrument.assigned)
        return failure(BAD_INSTRUMENT_INDEX);
    if (!export_sample(arctracker->module, instrument_no, output_filename))
        return failure(EXPORT_SAMPLE_FAILED);
    return SUCCESS;
}

static void *run_export(void *arg)
{
    arctracker_t *arctracker = arg;
    player_run(arctracker->export.player);
    arctracker->export_state = COMPLETE;
    return NULL;
}

api_result_t arctracker_player_shutdown(arctracker_t *arctracker)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (!arctracker->playback.thread_active)
        return failure(PLAYER_NOT_RUNNING);
    player_shutdown(arctracker->playback.player);
    pthread_join(arctracker->playback.audio_thread, NULL);
    arctracker->playback.thread_active = false;
    player_destroy(arctracker->playback.player);
    arctracker->playback.player = NULL;
    return SUCCESS;
}

api_result_t arctracker_export_cleanup(arctracker_t *arctracker)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (!arctracker->export.thread_active)
        return failure(EXPORT_NOT_STARTED);
    pthread_join(arctracker->export.audio_thread, NULL);
    arctracker->export.thread_active = false;
    player_destroy(arctracker->export.player);
    arctracker->export.player = NULL;
    arctracker->export_state = IDLE;
    return SUCCESS;
}

api_result_t arctracker_edit_get_event(arctracker_t *arctracker, int pattern_no, int pattern_index, int track, ui_pattern_event_t *event_buffer)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (event_buffer == NULL)
        return failure(BAD_EVENT_BUFFER);
    event_t event = {0};
    edit_result_t result = editor_get_event(arctracker->module, pattern_no, pattern_index, track, &event);
    if (!result.success)
        return failure(result.error_message);
    to_ui_event(event_buffer, &event);
    return SUCCESS;
}

api_result_t arctracker_edit_set_event(arctracker_t *arctracker, int pattern_no, int pattern_index, int track, ui_pattern_event_t *event)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (event == NULL)
        return failure(BAD_EVENT_BUFFER);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    event_t internal_event;
    to_internal_event(&internal_event, event);
    edit_result_t result = editor_set_event(arctracker->module, pattern_no, pattern_index, track, &internal_event);
    if (!result.success)
        return failure(result.error_message);
    return SUCCESS;
}

api_result_t arctracker_edit_get_sequence(arctracker_t *arctracker, int *sequence, int expected_sequence_len)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (sequence == NULL)
        return failure(BAD_SEQUENCE_BUFFER);
    module_t *module = arctracker->module;
    if (expected_sequence_len != module->sequence_length)
        return failure(INVALID_SEQUENCE_LENGTH);
    memcpy(sequence, module->sequence, module->sequence_length * sizeof(int));
    return SUCCESS;
}

api_result_t arctracker_edit_set_sequence(arctracker_t *arctracker, const int *new_sequence, const int new_sequence_len)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (new_sequence == NULL)
        return failure(BAD_SEQUENCE_BUFFER);
    if (new_sequence_len <= 0)
        return failure(INVALID_SEQUENCE_LENGTH);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    edit_result_t result = editor_set_sequence(arctracker->module, new_sequence, new_sequence_len);
    if (!result.success)
        return failure(result.error_message);
    player_sequence_changed(arctracker->playback.player, arctracker->module);
    return SUCCESS;
}

api_result_t arctracker_edit_create_pattern(arctracker_t *arctracker, const int pattern_length, int *new_pattern_no)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (pattern_length < 1 || pattern_length > MAX_PATTERN_LENGTH)
        return failure(INVALID_PATTERN_LENGTH);
    if (new_pattern_no == NULL)
        return failure(BAD_BUFFER);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    edit_result_t result = editor_create_pattern(arctracker->module, pattern_length, new_pattern_no);
    if (!result.success)
        return failure(result.error_message);
    return SUCCESS;
}

api_result_t arctracker_edit_delete_pattern(arctracker_t *arctracker, const int pattern_no)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (pattern_no != arctracker->module->num_patterns - 1)
        return failure(INVALID_PATTERN_NUMBER);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    edit_result_t result = editor_delete_pattern(arctracker->module, pattern_no);
    if (!result.success)
        return failure(result.error_message);
    return SUCCESS;
}

api_result_t arctracker_edit_set_pattern_length(arctracker_t *arctracker, const int pattern_no, const int new_length)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (pattern_no < 0 || pattern_no >= arctracker->module->num_patterns)
        return failure(INVALID_PATTERN_NUMBER);
    if (new_length < 1 || new_length > MAX_PATTERN_LENGTH)
        return failure(INVALID_PATTERN_LENGTH);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    const edit_result_t result = editor_set_pattern_length(arctracker->module, pattern_no, new_length);
    if (!result.success)
        return failure(result.error_message);
    return SUCCESS;
}

api_result_t arctracker_edit_set_instrument(arctracker_t *arctracker, const uint8_t slot, const ui_instrument_update_t instrument_update)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    if (instrument_update.assigned)
    {
        if (instrument_update.sample_index >= arctracker->module->sample_slots)
            return failure(INVALID_SAMPLE_INDEX);
        const sample_t sample = arctracker->module->samples[instrument_update.sample_index];
        if (sample.sample_length == 0)
            return failure(INVALID_SAMPLE_INDEX);
    }
    const sample_t sample = arctracker->module->samples[instrument_update.sample_index];
    if (instrument_update.repeat_offset < 0 || instrument_update.repeat_offset >= sample.sample_length - 1)
        return failure(INVALID_REPEAT_OFFSET);
    if (instrument_update.repeat_offset + instrument_update.repeat_length >= sample.sample_length)
        return failure(INVALID_REPEAT_LENGTH);
    const edit_result_t result = editor_update_instrument(
        arctracker->module,
        slot,
        instrument_update.assigned,
        instrument_update.name,
        instrument_update.default_volume,
        instrument_update.transpose,
        instrument_update.repeats,
        instrument_update.repeat_offset,
        instrument_update.repeat_length,
        instrument_update.sample_index);
    if (!result.success)
        return failure(result.error_message);
    return SUCCESS;
}

api_result_t arctracker_edit_load_sample(arctracker_t *arctracker, const char *filename, ui_sample_info_t *sample_info)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    if (sample_info == NULL)
        return failure(BAD_BUFFER);
    int sample_index;
    int sample_length;
    const edit_result_t result = editor_load_sample(arctracker->module, filename, &sample_index, &sample_length);
    if (!result.success)
        return failure(result.error_message);
    sample_info->sample_index = sample_index;
    sample_info->sample_length = sample_length;
    return SUCCESS;
}

api_result_t arctracker_edit_set_module_title(const arctracker_t *arctracker, const char *name, const char *author, const int default_pattern_length)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (name == NULL)
        return failure(MODULE_NAME_MISSING);
    if (strlen(name) > MAX_LEN_TUNENAME)
        return failure(MODULE_NAME_TOO_LONG);
    if (author == NULL)
        return failure(AUTHOR_MISSING);
    if (strlen(author) > MAX_LEN_AUTHOR)
        return failure(AUTHOR_TOO_LONG);
    if (default_pattern_length < 1 || default_pattern_length > MAX_PATTERN_LENGTH)
        return failure(INVALID_PATTERN_LENGTH);
    const edit_result_t result = editor_set_module_title(arctracker->module, name, author, default_pattern_length);
    if (!result.success)
        return failure(result.error_message);
    return SUCCESS;
}

api_result_t arctracker_edit_set_num_tracks(arctracker_t *arctracker, const int num_tracks)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    if (num_tracks < 1 || num_tracks > MAX_TRACKS)
        return failure(INVALID_TRACK_COUNT);
    const sequence_t sequence = arctracker->playback.player->sequence;
    if (arctracker->playback.thread_active)
        arctracker_player_shutdown(arctracker);
    const edit_result_t edit_result = editor_set_num_tracks(arctracker->module, num_tracks);
    if (!edit_result.success)
        return failure(edit_result.error_message);
    api_result_t restart_result = arctracker_player_start(arctracker);
    player_sequence_restore(arctracker->playback.player, sequence);
    if (!restart_result.success)
        return failure(restart_result.error_message);
    return SUCCESS;
}

api_result_t arctracker_edit_set_tempo(arctracker_t *arctracker, const uint8_t lines_per_beat, const uint8_t beats_per_minute)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->module == NULL)
        return failure(NO_MODULE_LOADED);
    if (arctracker->playback.player->playing)
        return failure(PLAYER_PLAYING);
    if (beats_per_minute > 0 && lines_per_beat == 0)
        return failure(TEMPO_UNDEFINED);
    module_set_lines_per_beat(arctracker->module, lines_per_beat);
    module_set_initial_bpm(arctracker->module, beats_per_minute);
    player_set_bpm(arctracker->playback.player, beats_per_minute);
    return SUCCESS;
}

api_result_t arctracker_destroy(arctracker_t *arctracker)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->playback.thread_active)
    {
        const api_result_t result = arctracker_player_shutdown(arctracker);
        if (!result.success)
            fprintf(stderr, "%s\n", result.error_message);
    }
    if (arctracker->module != NULL)
        remove_module(arctracker);
    event_queue_destroy(arctracker->playback.event_queue);
    event_queue_destroy(arctracker->export.event_queue);
    if (arctracker->playback.initialised && !stop_portaudio())
    {
        // PortAudio returned an error code when terminating, but the utility of that is only
        // that it might indicate a bug in Arctracker. It is not worth reporting to the user.
        fprintf(stderr, "%s\n", get_error_message());
    }
    deallocate(MAIN, arctracker);
    count_resources(MAIN, "main");
    count_resources(MODULE, "module");
    count_resources(PLAYER, "player");
    count_resources(AUDIO, "audio");
    return SUCCESS;
}

void arctracker_midi_destroy(midi_subsystem_t *midi)
{
    midi_destroy(midi);
}

static void remove_module(arctracker_t *arctracker)
{
    module_destroy(arctracker->module);
    arctracker->module = NULL;
}

static api_result_t failure(char *error_message)
{
    api_result_t result = {
        .success = false,
        .error_message = {0},
    };
    if (has_error())
        snprintf(result.error_message, sizeof result.error_message, "%s: (%s)", error_message, get_error_message());
    else
        snprintf(result.error_message, sizeof result.error_message, "%s", error_message);
    clear_error_state();
    return result;
}

static void count_resources(resource_group_t group, char *group_name)
{
    const int resource_count = resource_count_for(group);
    if (resource_count > 0)
        fprintf(stderr, "WARNING: %d %s resources were not deallocated\n", resource_count, group_name);
}
