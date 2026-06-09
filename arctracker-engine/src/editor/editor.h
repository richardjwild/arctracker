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

edit_result_t editor_get_event(module_t *module, int pattern_no, int pattern_index, int channel_no, event_t *event);

edit_result_t editor_set_event(module_t *module, int pattern_no, int pattern_index, int channel_no, event_t *event);

edit_result_t editor_set_sequence(module_t *module, const int *new_sequence, int new_sequence_len);

edit_result_t editor_create_pattern(module_t *module, int pattern_length, int *new_pattern_no);

edit_result_t editor_delete_pattern(module_t *module, int pattern_no);

#endif //ARCTRACKER_ENGINE_EDITOR_H
