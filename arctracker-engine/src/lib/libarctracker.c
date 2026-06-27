#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "libarctracker.h"
#include "arctracker.h"
#include "messages.h"
#include "player/player.h"
#include "audio_api/api.h"
#include "io/error.h"
#include "loader/loader.h"
#include "memory/heap.h"
#include "memory/bits.h"
#include "editor/editor.h"

#define SUCCESS (api_result_t) {\
    .success = true,\
    .error_message = {0}\
}
#define MAX_PATTERN_LENGTH 1000

static void *run_player(void *);
static void get_transport_state(player_t *player, module_t *module, ui_transport_state_t *transport_state);
static void copy_pattern_line(ui_pattern_event_t *line_buffer, event_t *events, int requested_tracks, int module_tracks);
static void to_ui_event(ui_pattern_event_t *event_buffer, event_t *event);
static void to_internal_event(event_t *event_buffer, ui_pattern_event_t *event);
static void *run_export(void *);
static void remove_module(arctracker_t *);
static api_result_t failure(char *);
static void count_resources(resource_group_t, char *);

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
    return arctracker;
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

api_result_t arctracker_module_create(arctracker_t *arctracker, int num_tracks, ui_module_info_t *module_info)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    module_t *module = module_create(num_tracks, 1, 1, 0);
    if (module == NULL || !module_init(module))
        return failure(MODULE_CREATE_FAILED);
    strcpy(module->name, NEW_MODULE_TITLE);
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
    if (arctracker->playback.thread_active)
        return failure(PLAYER_ALREADY_RUNNING);
    audio_api_t audio_api = create_audio_api(false, NULL);
    if (has_error())
    {
        return failure(AUDIO_INIT_FAILED);
    }
    arctracker->playback.player = player_create(arctracker->module, audio_api, arctracker->playback.event_queue);
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
    if (arctracker == NULL || !arctracker->playback.thread_active)
    {
        transport_state->playing = false;
        transport_state->looping = false;
        transport_state->sequence_pos = 0;
        transport_state->pattern_index = 0;
        transport_state->pattern_no = 0;
        transport_state->pattern_length = 0;
        return;
    }
    get_transport_state(arctracker->playback.player, arctracker->module, transport_state);
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
        export_state->percent_complete = (arctracker->export.player->sequence.sequence_pos * 100) / (arctracker->module->tune_length);
    }
}

static void get_transport_state(player_t *player, module_t *module, ui_transport_state_t *transport_state)
{
    const sequence_t sequence = player->sequence;
    const int pattern_no = module->sequence[sequence.sequence_pos];
    transport_state->playing = player->playing;
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
        events += module->num_tracks;
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
        event_buffer->effects[effect_no].command = effect.effect_code;
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
    audio_api_t audio_api = create_audio_api(true, output_filename);
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
    count_resources(PLAYER, "player");
    count_resources(AUDIO, "audio");
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
    if (expected_sequence_len != module->tune_length)
        return failure(INVALID_SEQUENCE_LENGTH);
    memcpy(sequence, module->sequence, module->tune_length * sizeof(int));
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
    if (instrument_update.assigned && instrument_update.sample_index >= arctracker->module->num_samples)
        return failure(INVALID_SAMPLE_INDEX);
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

api_result_t arctracker_edit_set_module_title(const arctracker_t *arctracker, const char *name, const char *author)
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
    const edit_result_t result = editor_set_module_title(arctracker->module, name, author);
    if (!result.success)
        return failure(result.error_message);
    printf("Changed module title to %s by %s\n", arctracker->module->name, arctracker->module->author);
    return SUCCESS;
}

api_result_t arctracker_destroy(arctracker_t *arctracker)
{
    if (arctracker == NULL)
        return failure(BAD_ARCTRACKER_HANDLE);
    if (arctracker->playback.thread_active)
        return failure(PLAYER_STILL_RUNNING);
    if (arctracker->module != NULL)
        remove_module(arctracker);
    event_queue_destroy(arctracker->playback.event_queue);
    event_queue_destroy(arctracker->export.event_queue);
    deallocate(MAIN, arctracker);
    count_resources(MAIN, "main");
    return SUCCESS;
}

static void remove_module(arctracker_t *arctracker)
{
    module_destroy(arctracker->module);
    count_resources(MODULE, "module");
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
    return result;
}

static void count_resources(resource_group_t group, char *group_name)
{
    if (resource_count_for(group) > 0)
        fprintf(stderr, "WARNING: %d %s resources were not deallocated\n", resource_count_for(group), group_name);
}
