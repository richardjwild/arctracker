#include "sequencer.h"

#define NOT_LOOPING (looping_state_t) {\
    .looping = false,\
    .loop_sequence_pos = 0,\
    .loop_pattern_start = 0,\
    .loop_pattern_end = 0,\
}

static const int NO_JUMP = -1;

static bool end_of_sequence(const sequence_t *);
static void advance_sequence_position(sequence_t *);
static bool end_of_pattern(const sequence_t *);
static bool end_of_loop(const sequence_t *);
static void advance_pattern_event(sequence_t *, bool *);
static void go_to_jump_target(sequence_t *);
static bool jump_permitted(int, const sequence_t *);

sequence_t initialise_sequence(const module_t *module, const bool bouncing)
{
    return (sequence_t) {
        .sequence_pos = 0,
        .looping_state = NOT_LOOPING,
        .pattern_index = 0,
        .jump_target = NO_JUMP,
        .jump_pattern_index = 0,
        .sequence = module->sequence,
        .tune_length = module->sequence_length,
        .patterns = module->patterns,
        .continuous_play = !bouncing,
    };
}

sequence_t reinitialise_sequence(const module_t *module, const sequence_t *old_sequence, const bool bouncing)
{
    sequence_t sequence = (sequence_t) {
        .sequence_pos = old_sequence->sequence_pos,
        .looping_state = old_sequence->looping_state,
        .pattern_index = 0,
        .jump_target = NO_JUMP,
        .sequence = module->sequence,
        .tune_length = module->sequence_length,
        .patterns = module->patterns,
        .continuous_play = !bouncing,
    };
    if (sequence.sequence_pos >= sequence.tune_length)
        sequence.sequence_pos = sequence.tune_length - 1;
    return sequence;
}

void pattern_step(sequence_t *sequence, bool *sequence_advanced)
{
    if (sequence->jump_target == NO_JUMP || sequence->looping_state.looping)
        advance_pattern_event(sequence, sequence_advanced);
    else
        go_to_jump_target(sequence);
}

void set_jump_target(const int next_position, const int jump_pattern_index, sequence_t *sequence)
{
    if (jump_permitted(next_position, sequence))
    {
        sequence->jump_target = next_position;
        sequence->jump_pattern_index = jump_pattern_index;
    }
}

void set_pattern_loop(sequence_t *sequence)
{
    const int current_sequence_pos = sequence->sequence_pos;
    const int current_pattern = sequence->sequence[current_sequence_pos];
    set_loop(sequence, 0, sequence->patterns[current_pattern].num_lines - 1);
}

void set_loop(sequence_t *sequence, const int loop_pattern_start, const int loop_pattern_end)
{
    const int current_sequence_pos = sequence->sequence_pos;
    sequence->looping_state.looping = true;
    sequence->looping_state.loop_sequence_pos = current_sequence_pos;
    sequence->looping_state.loop_pattern_start = loop_pattern_start;
    sequence->looping_state.loop_pattern_end = loop_pattern_end;
}

void clear_pattern_loop(sequence_t *sequence)
{
    sequence->looping_state = NOT_LOOPING;
}

void break_to_next_position(sequence_t *sequence, const int jump_pattern_index)
{
    if (sequence->looping_state.looping)
    {
        sequence->looping_state.loop_pattern_end = sequence->pattern_index;
        return;
    }
    set_jump_target(sequence->sequence_pos + 1, jump_pattern_index, sequence);
}

void sequence_seek(sequence_t *sequence, const int new_sequence_pos, const int new_pattern_pos)
{
    sequence->sequence_pos = new_sequence_pos;
    sequence->pattern_index = new_pattern_pos;
}

static void advance_sequence_position(sequence_t *sequence)
{
    sequence->sequence_pos += 1;
    if (end_of_sequence(sequence))
    {
        if (sequence->continuous_play)
            sequence->sequence_pos = 0;
        else
            sequence->song_ended = true;
    }
    sequence->pattern_index = 0;
}

static bool end_of_sequence(const sequence_t *sequence)
{
    return sequence->sequence_pos == sequence->tune_length;
}

static void advance_pattern_event(sequence_t *sequence, bool *sequence_advanced)
{
    if (sequence->looping_state.looping && end_of_loop(sequence))
    {
        sequence->pattern_index = sequence->looping_state.loop_pattern_start;
        return;
    }
    sequence->pattern_index += 1;
    if (end_of_pattern(sequence))
    {
        advance_sequence_position(sequence);
        *sequence_advanced = true;
    }
}

static bool end_of_pattern(const sequence_t *sequence)
{
    const int current_pattern = sequence->sequence[sequence->sequence_pos];
    const int pattern_length = sequence->patterns[current_pattern].num_lines;
    return sequence->pattern_index == pattern_length;
}

static bool end_of_loop(const sequence_t *sequence)
{
    return sequence->pattern_index == sequence->looping_state.loop_pattern_end;
}

static void go_to_jump_target(sequence_t *sequence)
{
    sequence->sequence_pos = sequence->jump_target;
    sequence->pattern_index = sequence->jump_pattern_index;
    sequence->jump_target = NO_JUMP;
    sequence->jump_pattern_index = 0;
}

static bool jump_permitted(const int next_position, const sequence_t *sequence)
{
    return next_position < sequence->tune_length
           && (sequence->continuous_play || next_position > sequence->sequence_pos);
}
