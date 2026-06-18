#ifndef ARCTRACKER_ENGINE_EDITOR_H
#define ARCTRACKER_ENGINE_EDITOR_H

#include "player/module.h"

#define EDIT_SUCCESS (edit_result_t) {\
    .success = true\
}

typedef struct {
    bool success;
    char *error_message;
} edit_result_t;

edit_result_t editor_get_event(const module_t *module, int pattern_no, int pattern_index, int channel_no, event_t *event_buffer);

edit_result_t editor_set_event(const module_t *module, int pattern_no, int pattern_index, int channel_no, const event_t *new_event);

edit_result_t editor_set_sequence(module_t *module, const int *new_sequence, int new_sequence_len);

edit_result_t editor_create_pattern(module_t *module, int pattern_length, int *new_pattern_no);

edit_result_t editor_delete_pattern(module_t *module, int pattern_no);

edit_result_t editor_update_instrument(
    module_t *module,
    uint8_t instrument_index,
    bool assigned,
    const char *name,
    uint8_t default_volume,
    int transpose,
    bool repeats,
    int repeat_offset,
    int repeat_length,
    int sample_index
);

edit_result_t editor_load_sample(module_t *module, const char *filename, int *sample_index, int *sample_length);

#endif //ARCTRACKER_ENGINE_EDITOR_H
