#include "sequence.h"

static int position_in_sequence;
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

void next_event()
{
    int current_pattern = sequence[position_in_sequence];
    int pattern_length = pattern_lengths[current_pattern];
    position_in_pattern += 1;
    if (position_in_pattern == pattern_length)
    {
        position_in_sequence += 1;
        position_in_pattern = 0;
        if (position_in_sequence == tune_length)
        {
            position_in_sequence = 0;
            looped = true;
        }
        int next_pattern = sequence[position_in_sequence];
        pattern_line_ptr = patterns[next_pattern];
    }
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

void break_to_next_position()
{
    int current_pattern = sequence[position_in_sequence];
    position_in_pattern = pattern_lengths[current_pattern] - 1;
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
    {
        position_in_sequence = next_position - 1;
        int current_pattern = sequence[position_in_sequence];
        position_in_pattern = pattern_lengths[current_pattern] - 1;
    }
}