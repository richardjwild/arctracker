#include "sequencer.h"

static const looping_state_t NOT_LOOPING = {
    .looping = false,
};

static const sequence_jump_t NO_SEQUENCE_JUMP = {
    .commanded = false,
};

static const pattern_break_t NO_PATTERN_BREAK = {
    .commanded = false,
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
        .sequence = module->sequence,
        .tune_length = module->sequence_length,
        .patterns = module->patterns,
        .sequence_index = 0,
        .pattern_index = 0,
        .looping_state = NOT_LOOPING,
        .sequence_jump = NO_SEQUENCE_JUMP,
        .pattern_break = NO_PATTERN_BREAK,
        .continuous_play = !bouncing,
    };
}

sequence_t sequencer_reinitialise(const module_t *module, const sequence_t *old_sequence, const bool bouncing)
{
    sequence_t sequence = (sequence_t) {
        .sequence = module->sequence,
        .tune_length = module->sequence_length,
        .patterns = module->patterns,
        .sequence_index = old_sequence->sequence_index,
        .pattern_index = 0,
        .looping_state = old_sequence->looping_state,
        .sequence_jump = NO_SEQUENCE_JUMP,
        .pattern_break = NO_PATTERN_BREAK,
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

void sequencer_set_next_sequence_index(sequence_t *sequence, const int sequence_index)
{
    if (jump_permitted(sequence_index, sequence))
    {
        sequence->sequence_jump.commanded = true;
        sequence->sequence_jump.sequence_index = sequence_index;
    }
}

void sequencer_set_whole_pattern_loop(sequence_t *sequence)
{
    const int current_sequence_index = sequence->sequence_index;
    const int current_pattern = sequence->sequence[current_sequence_index];
    const int last_pattern_index = sequence->patterns[current_pattern].num_lines - 1;
    sequencer_set_loop(sequence, 0, last_pattern_index, true);
}

void sequencer_set_loop(sequence_t *sequence, const int start_index, const int end_index, const bool commanded_by_ui)
{
    sequence->looping_state.looping = true;
    sequence->looping_state.sequence_index = sequence->sequence_index;
    sequence->looping_state.start_pattern_index = start_index;
    sequence->looping_state.end_pattern_index = end_index;
    sequence->looping_state.commanded_by_ui = commanded_by_ui;
}

void sequencer_clear_loop(sequence_t *sequence)
{
    sequence->looping_state = NOT_LOOPING;
}

void sequencer_break_pattern(sequence_t *sequence, const int entry_pattern_index)
{
    if (sequence->looping_state.looping && sequence->looping_state.commanded_by_ui)
    {
        // The sequence should not advance because we are in a loop commanded by the UI, but we also do not want the
        // playhead to progress past the break command. So we set the end of the loop to location of the pattern break.
        sequence->looping_state.end_pattern_index = sequence->pattern_index;
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
        sequence->pattern_index = sequence->looping_state.start_pattern_index;
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
    return sequence->pattern_index == sequence->looping_state.end_pattern_index;
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
