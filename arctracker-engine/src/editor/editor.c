#include "editor.h"
#include <string.h>
#include "messages.h"
#include "memory/heap.h"

static edit_result_t get_event(module_t *, int, int, int, event_t **);
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
    pattern_t *pattern = module->patterns[pattern_no];
    if (pattern == NULL || pattern->events == NULL)
        return failure(INVALID_PATTERN_NUMBER);
    if (pattern_index < 0 || pattern_index >= pattern->num_lines)
        return failure(INVALID_PATTERN_INDEX);
    *event = pattern->events + (pattern_index * module->num_channels) + channel_no;
    return EDIT_SUCCESS;
}

static edit_result_t failure(char *message)
{
    return (edit_result_t) {
        .success = false,
        .error_message = message,
    };
}
