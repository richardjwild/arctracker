#include "sequencer.h"

#define NOT_LOOPING (looping_state_t) {\
    .looping = false,\
    .loop_sequence_pos = 0,\
    .loop_pattern_start = 0,\
    .loop_pattern_end = 0,\
}

static const int NO_JUMP = -1;

static void begin_new_pattern(sequence_t *);
static bool end_of_sequence(sequence_t *);
static void advance_sequence_position(sequence_t *);
static bool end_of_pattern(sequence_t *);
static bool end_of_loop(sequence_t *);
static void advance_pattern_event(sequence_t *);
static void go_to_jump_target(sequence_t *);
static bool jump_permitted(int, sequence_t *);

sequence_t initialise_sequence(module_t *module, bool bouncing)
{
    sequence_t sequence = (sequence_t) {
        .sequence_pos = 0,
        .looping_state = NOT_LOOPING,
        .pattern_index = 0,
        .jump_target = NO_JUMP,
        .sequence = module->sequence,
        .tune_length = module->tune_length,
        .patterns = module->patterns,
        .allow_backwards_jump = !bouncing,
    };
    return sequence;
}

void sequence_advance(sequence_t *sequence)
{
    if (sequence->jump_target == NO_JUMP || sequence->looping_state.looping)
        advance_pattern_event(sequence);
    else
        go_to_jump_target(sequence);
}

void set_jump_target(int next_position, sequence_t *sequence)
{
    if (jump_permitted(next_position, sequence))
        sequence->jump_target = next_position;
}

void set_pattern_loop(sequence_t *sequence)
{
    const int current_sequence_pos = sequence->sequence_pos;
    const int current_pattern = sequence->sequence[current_sequence_pos];
    sequence->looping_state.looping = true;
    sequence->looping_state.loop_sequence_pos = current_sequence_pos;
    sequence->looping_state.loop_pattern_start = 0;
    sequence->looping_state.loop_pattern_end = sequence->patterns[current_pattern]->num_lines - 1;
}

void clear_pattern_loop(sequence_t *sequence)
{
    sequence->looping_state = NOT_LOOPING;
}

void break_to_next_position(sequence_t *sequence)
{
    if (sequence->looping_state.looping)
    {
        sequence->looping_state.loop_pattern_end = sequence->pattern_index;
        return;
    }
    set_jump_target(sequence->sequence_pos + 1, sequence);
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
        sequence->sequence_pos = 0;
        sequence->looped = true;
    }
    begin_new_pattern(sequence);
}

static bool end_of_sequence(sequence_t *sequence)
{
    return (sequence->sequence_pos == sequence->tune_length);
}

static void begin_new_pattern(sequence_t *sequence)
{
    sequence->pattern_index = 0;
}

static void advance_pattern_event(sequence_t *sequence)
{
    if (sequence->looping_state.looping && end_of_loop(sequence))
    {
        sequence->pattern_index = sequence->looping_state.loop_pattern_start;
        return;
    }
    sequence->pattern_index += 1;
    if (end_of_pattern(sequence))
        advance_sequence_position(sequence);
}

static bool end_of_pattern(sequence_t *sequence)
{
    int current_pattern = sequence->sequence[sequence->sequence_pos];
    int pattern_length = sequence->patterns[current_pattern]->num_lines;
    return (sequence->pattern_index == pattern_length);
}

static bool end_of_loop(sequence_t *sequence)
{
    return (sequence->pattern_index == sequence->looping_state.loop_pattern_end);
}

static void go_to_jump_target(sequence_t *sequence)
{
    sequence->sequence_pos = sequence->jump_target;
    sequence->jump_target = NO_JUMP;
    begin_new_pattern(sequence);
}

static bool jump_permitted(int next_position, sequence_t *sequence)
{
    return (next_position < sequence->tune_length)
           && (sequence->allow_backwards_jump || (next_position > sequence->sequence_pos));
}
