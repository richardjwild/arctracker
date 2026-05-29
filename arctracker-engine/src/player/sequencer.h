#ifndef ARCTRACKER_SEQUENCE_H
#define ARCTRACKER_SEQUENCE_H

#include <stdbool.h>
#include "module.h"

typedef struct {
    bool looping;
    int loop_sequence_pos;
    int loop_pattern_start;
    int loop_pattern_end;
} looping_state_t;

typedef struct {
    const int *sequence;
    looping_state_t looping_state;
    int tune_length;
    pattern_t **patterns;
    int sequence_pos;
    int pattern_index;
    int jump_target;
    bool looped;
    bool allow_backwards_jump;
} sequence_t;

sequence_t initialise_sequence(module_t *module, bool bouncing);

void sequence_advance(sequence_t *);

void sequence_seek(sequence_t *, int new_sequence_pos, int new_pattern_pos);

void break_to_next_position(sequence_t *);

void set_jump_target(int next_position, sequence_t *sequence);

void set_pattern_loop(sequence_t *);

void clear_pattern_loop(sequence_t *);

void sequence_destroy(sequence_t *sequence);

#endif //ARCTRACKER_SEQUENCE_H
