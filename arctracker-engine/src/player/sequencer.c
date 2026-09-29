#include "sequencer.h"

static const looping_state_t NOT_LOOPING = {
    .looping = false,
    .loop_sequence_index = 0,
    .loop_pattern_start = 0,
    .loop_pattern_end = 0,
};

static bool end_of_sequence(const sequence_t *);
static void advance_sequence_index(sequence_t *);
static bool end_of_pattern(const sequence_t *);
static bool end_of_loop(const sequence_t *);
static void advance_pattern_event(sequence_t *, bool *);
static void execute_commanded_pattern_entry(sequence_t *);
static bool jump_permitted(int, const sequence_t *);

sequence_t sequencer_initialise(const module_t *module, const bool bouncing)
{
    return (sequence_t) {
        .sequence_index = 0,
        .looping_state = NOT_LOOPING,
        .pattern_index = 0,
        .sequence_jump = (sequence_jump_t) {
            .commanded = false,
        },
        .pattern_break = (pattern_break_t) {
            .commanded = false,
        },
        .sequence = module->sequence,
        .tune_length = module->sequence_length,
        .patterns = module->patterns,
        .continuous_play = !bouncing,
    };
}

sequence_t sequencer_reinitialise(const module_t *module, const sequence_t *old_sequence, const bool bouncing)
{
    sequence_t sequence = (sequence_t) {
        .sequence_index = old_sequence->sequence_index,
        .looping_state = old_sequence->looping_state,
        .pattern_index = 0,
        .sequence_jump = (sequence_jump_t) {
            .commanded = false,
        },
        .pattern_break = (pattern_break_t) {
            .commanded = false,
        },
        .sequence = module->sequence,
        .tune_length = module->sequence_length,
        .patterns = module->patterns,
        .continuous_play = !bouncing,
    };
    if (sequence.sequence_index >= sequence.tune_length)
        sequence.sequence_index = sequence.tune_length - 1;
    return sequence;
}

void sequencer_advance(sequence_t *sequence, bool *pattern_entered)
{
    if ((sequence->sequence_jump.commanded || sequence->pattern_break.commanded) && !sequence->looping_state.looping)
    {
        execute_commanded_pattern_entry(sequence);
        *pattern_entered = true;
    }
    else
        advance_pattern_event(sequence, pattern_entered);
}

void sequencer_set_next_sequence_index(const int sequence_index, sequence_t *sequence)
{
    if (jump_permitted(sequence_index, sequence))
    {
        sequence->sequence_jump.commanded = true;
        sequence->sequence_jump.sequence_index = sequence_index;
    }
}

void sequencer_set_pattern_loop(sequence_t *sequence)
{
    const int current_sequence_index = sequence->sequence_index;
    const int current_pattern = sequence->sequence[current_sequence_index];
    sequencer_set_loop(sequence, 0, sequence->patterns[current_pattern].num_lines - 1, true);
}

void sequencer_set_loop(sequence_t *sequence, const int loop_pattern_start, const int loop_pattern_end, const bool commanded_by_ui)
{
    sequence->looping_state.looping = true;
    sequence->looping_state.loop_sequence_index = sequence->sequence_index;
    sequence->looping_state.loop_pattern_start = loop_pattern_start;
    sequence->looping_state.loop_pattern_end = loop_pattern_end;
    sequence->looping_state.commanded_by_ui = commanded_by_ui;
}

void sequencer_clear_pattern_loop(sequence_t *sequence)
{
    sequence->looping_state = NOT_LOOPING;
}

void sequencer_break_pattern(sequence_t *sequence, const int entry_pattern_index)
{
    if (sequence->looping_state.looping)
    {
        sequence->looping_state.loop_pattern_end = sequence->pattern_index;
        return;
    }
    sequence->pattern_break.commanded = true;
    sequence->pattern_break.pattern_index = entry_pattern_index;
}

void sequencer_seek(sequence_t *sequence, const int sequence_index, const int pattern_index)
{
    sequence->sequence_index = sequence_index;
    sequence->pattern_index = pattern_index;
}

static void advance_sequence_index(sequence_t *sequence)
{
    sequence->sequence_index += 1;
    if (end_of_sequence(sequence))
    {
        if (sequence->continuous_play)
            sequence->sequence_index = 0;
        else
            sequence->song_ended = true;
    }
    sequence->pattern_index = 0;
}

static bool end_of_sequence(const sequence_t *sequence)
{
    return sequence->sequence_index == sequence->tune_length;
}

static void advance_pattern_event(sequence_t *sequence, bool *pattern_entered)
{
    if (sequence->looping_state.looping && end_of_loop(sequence))
    {
        sequence->pattern_index = sequence->looping_state.loop_pattern_start;
        return;
    }
    sequence->pattern_index += 1;
    if (end_of_pattern(sequence))
    {
        advance_sequence_index(sequence);
        *pattern_entered = true;
    }
}

static bool end_of_pattern(const sequence_t *sequence)
{
    const int current_pattern = sequence->sequence[sequence->sequence_index];
    const int pattern_length = sequence->patterns[current_pattern].num_lines;
    return sequence->pattern_index == pattern_length;
}

static bool end_of_loop(const sequence_t *sequence)
{
    return sequence->pattern_index == sequence->looping_state.loop_pattern_end;
}

static void execute_commanded_pattern_entry(sequence_t *sequence)
{
    if (sequence->sequence_jump.commanded)
    {
        sequence->sequence_index = sequence->sequence_jump.sequence_index;
    }
    else
    {
        sequence->sequence_index += 1;
        if (end_of_sequence(sequence)) sequence->sequence_index = 0;
    }
    sequence->pattern_index = sequence->pattern_break.pattern_index;
    sequence->sequence_jump.commanded = false;
    sequence->pattern_break.commanded = false;
}

static bool jump_permitted(const int sequence_index, const sequence_t *sequence)
{
    return sequence_index < sequence->tune_length
           && (sequence->continuous_play || sequence_index > sequence->sequence_index);
}
