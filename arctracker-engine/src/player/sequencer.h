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
    pattern_t *patterns;
    int sequence_pos;
    int pattern_index;
    int jump_target;
    int jump_pattern_index;
    bool song_ended;
    bool continuous_play;
} sequence_t;

sequence_t initialise_sequence(const module_t *module, bool bouncing);

sequence_t reinitialise_sequence(const module_t *module, const sequence_t *old_sequence, bool bouncing);

void pattern_step(sequence_t *);

void sequence_seek(sequence_t *, int new_sequence_pos, int new_pattern_pos);

void break_to_next_position(sequence_t *, int jump_pattern_index);

void set_jump_target(int next_position, int jump_pattern_index, sequence_t *sequence);

void set_pattern_loop(sequence_t *);

void set_loop(sequence_t *, int, int);

void clear_pattern_loop(sequence_t *);

void sequence_destroy(sequence_t *sequence);

#endif //ARCTRACKER_SEQUENCE_H
