#ifndef ARCTRACKER_SEQUENCE_H
#define ARCTRACKER_SEQUENCE_H

#include <arctracker.h>

void initialise_sequence(module_t *module);

void forbid_jumping_backwards();

int song_position();

int pattern_position();

void next_event();

void *pattern_line();

void advance_pattern_line(size_t bytes);

bool looped_yet();

void break_to_next_position();

void jump_to_position(int next_position);

#endif //ARCTRACKER_SEQUENCE_H
