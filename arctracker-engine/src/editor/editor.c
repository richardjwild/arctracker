#include "editor.h"
#include <string.h>
#include "messages.h"
#include "io/error.h"
#include "memory/heap.h"

#define MIN_SEQUENCE_CAPACITY 1024
#define MIN_PATTERN_CAPACITY 1024
#define MAX_PATTERN_LENGTH 1000

static edit_result_t get_event(module_t *, int, int, int, event_t **);
static edit_result_t ensure_sequence_capacity(module_t *, int);
static edit_result_t ensure_pattern_capacity(module_t *module, int num_patterns);
static edit_result_t failure(char *);

edit_result_t editor_get_event(module_t *module, int pattern_no, int pattern_index, int channel_no, event_t *event_buffer)
{
    if (event_buffer == NULL)
        return failure(BAD_EVENT_BUFFER);
    event_t *event = NULL;
    edit_result_t result = get_event(module, pattern_no, pattern_index, channel_no, &event);
    if (!result.success)
        return result;
    *event_buffer = *event;
    return EDIT_SUCCESS;
}

edit_result_t editor_set_event(module_t *module, int pattern_no, int pattern_index, int channel_no, event_t *new_event)
{
    if (new_event == NULL)
        return failure(BAD_EVENT_BUFFER);
    event_t *existing_event = NULL;
    edit_result_t result = get_event(module, pattern_no, pattern_index, channel_no, &existing_event);
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
    edit_result_t result = ensure_sequence_capacity(module, new_sequence_len);
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
    edit_result_t result = ensure_pattern_capacity(module, module->num_patterns + 1);
    if (!result.success)
        return result;
    *new_pattern_no = module->num_patterns;
    if (!module_create_pattern(module, *new_pattern_no, pattern_length))
        return failure(MEMORY_ALLOCATION_FAILED);
    module->num_patterns++;
    return EDIT_SUCCESS;
}

static edit_result_t get_event(module_t *module, int pattern_no, int pattern_index, int channel_no, event_t **event)
{
    *event = NULL;
    if (module == NULL)
        return failure(NO_MODULE_LOADED);
    if (pattern_no < 0 || pattern_no >= module->num_patterns)
        return failure(INVALID_PATTERN_NUMBER);
    if (channel_no < 0 || channel_no >= module->num_channels)
        return failure(INVALID_CHANNEL_NUMBER);
    if (module->patterns == NULL)
        return failure(NO_PATTERN_DATA);
    pattern_t pattern = module->patterns[pattern_no];
    if (pattern_index < 0 || pattern_index >= pattern.num_lines)
        return failure(INVALID_PATTERN_INDEX);
    *event = pattern.events + (pattern_index * module->num_channels) + channel_no;
    return EDIT_SUCCESS;
}

static edit_result_t ensure_sequence_capacity(module_t *module, int sequence_len)
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

static edit_result_t ensure_pattern_capacity(module_t *module, int num_patterns)
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

static edit_result_t failure(char *message)
{
    return (edit_result_t) {
        .success = false,
        .error_message = message,
    };
}
