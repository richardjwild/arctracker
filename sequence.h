#ifndef ARCTRACKER_SEQUENCE_H
#define ARCTRACKER_SEQUENCE_H

#include "arctracker.h"

void initialise_sequence(module_t *module);

int song_position();

int pattern_position();

void next_event();

void *pattern_line();

void advance_pattern_line(size_t bytes);

bool looped_yet();

#endif //ARCTRACKER_SEQUENCE_H
