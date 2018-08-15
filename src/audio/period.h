#ifndef ARCTRACKER_PERIOD_H
#define ARCTRACKER_PERIOD_H

#include <arctracker.h>

static const int PERIOD_MAX = 0x3f0;
static const int PERIOD_MIN = 0x50;
static const int LOWEST_NOTE = 0;
static const int HIGHEST_NOTE = 61;

#define NOTE_OUT_OF_RANGE(note) ((note) < LOWEST_NOTE || (note) > HIGHEST_NOTE)

unsigned int period_for_note(int note);

#endif //ARCTRACKER_PERIOD_H
