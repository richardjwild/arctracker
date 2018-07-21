#ifndef ARCTRACKER_PERIOD_H
#define ARCTRACKER_PERIOD_H

#include "arctracker.h"

#define PERIOD_MAX 0x3f0
#define PERIOD_MIN 0x50
#define LOWEST_NOTE 0
#define HIGHEST_NOTE 61
#define NOTE_OUT_OF_RANGE(note) (note) < LOWEST_NOTE || (note) > HIGHEST_NOTE

bool out_of_range(int note);

unsigned int period_for_note(int note);

#endif //ARCTRACKER_PERIOD_H
