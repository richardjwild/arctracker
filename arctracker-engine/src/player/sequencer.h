#ifndef ARCTRACKER_SEQUENCE_H
#define ARCTRACKER_SEQUENCE_H

#include <stdbool.h>
#include "module.h"

typedef struct {
    bool looping;
    bool commanded_by_ui;
    int sequence_index;
    int start_pattern_index;
    int end_pattern_index;
} looping_state_t;

typedef struct {
    bool commanded;
    int sequence_index;
} sequence_jump_t;

typedef struct {
    bool commanded;
    int pattern_index;
} pattern_break_t;

typedef struct {
    const int *sequence;
    int tune_length;
    pattern_t *patterns;
    int sequence_index;
    int pattern_index;
    looping_state_t looping_state;
    sequence_jump_t sequence_jump;
    pattern_break_t pattern_break;
    int restart_position;
    bool song_ended;
    bool continuous_play;
} sequence_t;

sequence_t sequencer_initialise(const module_t *, bool bouncing);

sequence_t sequencer_reinitialise(const module_t *, const sequence_t *old_sequence, bool bouncing);

void sequencer_advance(sequence_t *, bool *pattern_entered);

void sequencer_seek(sequence_t *, int sequence_index, int pattern_index);

void sequencer_break_pattern(sequence_t *, int entry_pattern_index);

void sequencer_set_next_sequence_index(sequence_t *, int sequence_index);

void sequencer_set_whole_pattern_loop(sequence_t *);

void sequencer_set_loop(sequence_t *, int start_index, int end_index, bool commanded_by_ui);

void sequencer_clear_loop(sequence_t *);

void sequencer_destroy(sequence_t *);

#endif //ARCTRACKER_SEQUENCE_H
