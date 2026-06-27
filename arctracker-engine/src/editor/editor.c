#include "editor.h"

#include <stdio.h>
#include <string.h>
#include "messages.h"
#include "io/error.h"
#include "loader/loader.h"
#include "memory/heap.h"

#define MIN_SEQUENCE_CAPACITY 1024
#define MIN_PATTERN_CAPACITY 1024
#define MIN_SAMPLE_CAPACITY 1024
#define MAX_PATTERN_LENGTH 1000

static edit_result_t get_event(const module_t *, int, int, int, event_t **);
static edit_result_t ensure_sequence_capacity(module_t *, int);
static edit_result_t ensure_pattern_capacity(module_t *module, int num_patterns);
static edit_result_t ensure_sample_capacity(module_t *module, int num_samples);
static edit_result_t failure(char *);

edit_result_t editor_get_event(const module_t *module, const int pattern_no, const int pattern_index, const int track, event_t *event_buffer)
{
    if (event_buffer == NULL)
        return failure(BAD_EVENT_BUFFER);
    event_t *event = NULL;
    const edit_result_t result = get_event(module, pattern_no, pattern_index, track, &event);
    if (!result.success)
        return result;
    *event_buffer = *event;
    return EDIT_SUCCESS;
}

edit_result_t editor_set_event(const module_t *module, const int pattern_no, const int pattern_index, const int track, const event_t *new_event)
{
    if (new_event == NULL)
        return failure(BAD_EVENT_BUFFER);
    event_t *existing_event = NULL;
    const edit_result_t result = get_event(module, pattern_no, pattern_index, track, &existing_event);
    if (!result.success)
        return result;
    *existing_event = *new_event;
    return result;
}

edit_result_t editor_set_sequence(module_t *module, const int *new_sequence, const int new_sequence_len)
{
    if (new_sequence == NULL)
        return failure(BAD_SEQUENCE_BUFFER);
    if (new_sequence_len <= 0)
        return failure(INVALID_SEQUENCE_LENGTH);
    for (int i = 0; i < new_sequence_len; i++) {
        if (new_sequence[i] < 0 || new_sequence[i] >= module->num_patterns)
            return failure(INVALID_PATTERN_NUMBER);
    }
    const edit_result_t result = ensure_sequence_capacity(module, new_sequence_len);
    if (!result.success)
        return result;
    memcpy(module->sequence, new_sequence, new_sequence_len * sizeof(int));
    module->tune_length = new_sequence_len;
    return EDIT_SUCCESS;
}

edit_result_t editor_create_pattern(module_t *module, const int pattern_length, int *new_pattern_no)
{
    if (pattern_length < 1 || pattern_length > MAX_PATTERN_LENGTH)
        return failure(INVALID_PATTERN_LENGTH);
    const edit_result_t result = ensure_pattern_capacity(module, module->num_patterns + 1);
    if (!result.success)
        return result;
    *new_pattern_no = module->num_patterns;
    if (!module_create_pattern(module, *new_pattern_no, pattern_length))
        return failure(MEMORY_ALLOCATION_FAILED);
    module->num_patterns++;
    return EDIT_SUCCESS;
}

edit_result_t editor_delete_pattern(module_t *module, const int pattern_no)
{
    module_delete_pattern(module, pattern_no);
    return EDIT_SUCCESS;
}

edit_result_t editor_set_pattern_length(const module_t *module, const int pattern_no, const int pattern_length)
{
    if (pattern_length < 1 || pattern_length > MAX_PATTERN_LENGTH)
        return failure(INVALID_PATTERN_LENGTH);
    if (!module_set_pattern_length(module, pattern_no, pattern_length))
        return failure(MEMORY_ALLOCATION_FAILED);
    return EDIT_SUCCESS;
}

edit_result_t editor_update_instrument(
    module_t *module,
    const uint8_t instrument_index,
    const bool assigned,
    const char *name,
    const uint8_t default_volume,
    const int transpose,
    const bool repeats,
    const int repeat_offset,
    const int repeat_length,
    const int sample_index
) {
    instrument_t instrument = {0};
    snprintf(instrument.name, sizeof instrument.name, "%s", name);
    instrument.default_volume = default_volume;
    instrument.transpose = transpose;
    instrument.repeats = repeats;
    if (repeats)
    {
        instrument.repeat_offset = repeat_offset;
        instrument.repeat_length = repeat_length;
    }
    instrument.assigned = assigned;
    if (instrument.assigned)
    {
        instrument.sample_index = sample_index;
    }
    module_set_instrument(module, instrument_index, instrument);
    return EDIT_SUCCESS;
}

edit_result_t editor_load_sample(module_t *module, const char *filename, int *sample_index, int *sample_length)
{
    const load_sample_result_t load_result = load_sample(filename);
    if (!load_result.file_read)
        return failure(FILE_OPEN_FAILED);
    if (!load_result.file_valid)
        return failure(SAMPLE_LOAD_FAILED);
    const edit_result_t capacity_result = ensure_sample_capacity(module, module->num_patterns + 1);
    if (!capacity_result.success)
    {
        deallocate(MODULE, load_result.sample_data);
        return capacity_result;
    }
    if (!module_link_sample(module, load_result.sample_data, load_result.sample_length, sample_index))
    {
        deallocate(MODULE, load_result.sample_data);
        return failure(SAMPLE_LINK_FAILED);
    }
    *sample_length = load_result.sample_length;
    return EDIT_SUCCESS;
}

edit_result_t editor_set_module_title(module_t *module, const char *name, const char *author)
{
    module_set_name(module, name);
    module_set_author(module, author);
    return EDIT_SUCCESS;
}

static edit_result_t get_event(const module_t *module, const int pattern_no, const int pattern_index, const int track, event_t **event)
{
    *event = NULL;
    if (module == NULL)
        return failure(NO_MODULE_LOADED);
    if (pattern_no < 0 || pattern_no >= module->num_patterns)
        return failure(INVALID_PATTERN_NUMBER);
    if (track < 0 || track >= module->num_tracks)
        return failure(INVALID_TRACK_NUMBER);
    if (module->patterns == NULL)
        return failure(NO_PATTERN_DATA);
    const pattern_t pattern = module->patterns[pattern_no];
    if (pattern_index < 0 || pattern_index >= pattern.num_lines)
        return failure(INVALID_PATTERN_INDEX);
    *event = pattern.events + pattern_index * module->num_tracks + track;
    return EDIT_SUCCESS;
}

static edit_result_t ensure_sequence_capacity(module_t *module, const int sequence_len)
{
    if (module->sequence_capacity >= sequence_len)
        return EDIT_SUCCESS;
    int required_capacity = module->sequence_capacity;
    while (required_capacity < sequence_len)
        required_capacity += MIN_SEQUENCE_CAPACITY;
    int *new_sequence = reallocate_array(MODULE, module->sequence, required_capacity, sizeof(int));
    if (new_sequence == NULL)
        return failure(MEMORY_ALLOCATION_FAILED);
    module->sequence = new_sequence;
    module->sequence_capacity = required_capacity;
    return EDIT_SUCCESS;
}

static edit_result_t ensure_pattern_capacity(module_t *module, const int num_patterns)
{
    if (module->pattern_capacity >= num_patterns)
        return EDIT_SUCCESS;
    int required_capacity = module->pattern_capacity;
    while (required_capacity < num_patterns)
        required_capacity += MIN_PATTERN_CAPACITY;
    pattern_t *new_patterns = reallocate_array(MODULE, module->patterns, required_capacity, sizeof(pattern_t));
    if (new_patterns == NULL)
        return failure(MEMORY_ALLOCATION_FAILED);
    module->patterns = new_patterns;
    module->pattern_capacity = required_capacity;
    return EDIT_SUCCESS;
}

static edit_result_t ensure_sample_capacity(module_t *module, const int num_samples)
{
    if (module->sample_capacity >= num_samples)
        return EDIT_SUCCESS;
    int required_capacity = module->sample_capacity;
    while (required_capacity < num_samples)
        required_capacity += MIN_SAMPLE_CAPACITY;
    sample_t *new_samples = reallocate_array(MODULE, module->samples, required_capacity, sizeof(sample_t));
    if (new_samples == NULL)
        return failure(MEMORY_ALLOCATION_FAILED);
    module->samples = new_samples;
    module->sample_capacity = required_capacity;
    return EDIT_SUCCESS;
}

static edit_result_t failure(char *message)
{
    return (edit_result_t) {
        .success = false,
        .error_message = message,
    };
}
