#include "sequence.h"
#include "arctracker.h"

static int position_in_sequence;
static int position_in_pattern;
static void *pattern_line_ptr;
static const unsigned char *sequence;
static long tune_length;
static void **patterns;
static unsigned char *pattern_lengths;
static bool looped;

void initialise_sequence(module_t *module)
{
    position_in_sequence = 0;
    position_in_pattern = -1;
    sequence = module->sequence;
    tune_length = module->tune_length;
    patterns = module->patterns;
    pattern_lengths = module->pattern_length;
    unsigned char first_pattern = sequence[position_in_sequence];
    pattern_line_ptr = patterns[first_pattern];
    looped = false;
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