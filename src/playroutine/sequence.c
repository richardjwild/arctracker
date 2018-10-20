#include "sequence.h"

static const int NO_JUMP = -1;

static int position_in_sequence;
static int jump_position_target;
static int position_in_pattern;
static void *pattern_line_ptr;
static const int *sequence;
static long tune_length;
static void **patterns;
static int *pattern_lengths;
static bool looped;
static bool jump_backwards_permitted;

void initialise_sequence(module_t *module)
{
    position_in_sequence = 0;
    position_in_pattern = -1;
    jump_position_target = NO_JUMP;
    sequence = module->sequence;
    tune_length = module->tune_length;
    patterns = module->patterns;
    pattern_lengths = module->pattern_lengths;
    int first_pattern = sequence[position_in_sequence];
    pattern_line_ptr = patterns[first_pattern];
    looped = false;
    jump_backwards_permitted = true;
}

void forbid_jumping_backwards()
{
    jump_backwards_permitted = false;
}

int song_position()
{
    return position_in_sequence;
}

int pattern_position()
{
    return position_in_pattern;
}

void begin_new_pattern()
{
    int next_pattern = sequence[position_in_sequence];
    pattern_line_ptr = patterns[next_pattern];
    position_in_pattern = 0;
}

static inline
bool end_of_sequence()
{
    return (position_in_sequence == tune_length);
}

void advance_sequence_position()
{
    position_in_sequence += 1;
    if (end_of_sequence())
    {
        position_in_sequence = 0;
        looped = true;
    }
    begin_new_pattern();
}

static inline
bool end_of_pattern()
{
    int current_pattern = sequence[position_in_sequence];
    int pattern_length = pattern_lengths[current_pattern];
    return (position_in_pattern == pattern_length);
}

void advance_pattern_event()
{
    position_in_pattern += 1;
    if (end_of_pattern())
        advance_sequence_position();
}

void go_to_jump_target()
{
    position_in_sequence = jump_position_target;
    jump_position_target = NO_JUMP;
    begin_new_pattern();
}

void next_event()
{
    if (jump_position_target == NO_JUMP)
        advance_pattern_event();
    else
        go_to_jump_target();
}

void *pattern_line()
{
    return pattern_line_ptr;
}

void advance_pattern_line(size_t bytes)
{
    pattern_line_ptr += bytes;
}

bool looped_yet()
{
    return looped;
}

static inline
bool jump_permitted(int next_position)
{
    return (next_position < tune_length)
           && (jump_backwards_permitted || (next_position > position_in_sequence));
}

void jump_to_position(int next_position)
{
    if (jump_permitted(next_position))
        jump_position_target = next_position;
}

void break_to_next_position()
{
    jump_to_position(position_in_sequence + 1);
}
