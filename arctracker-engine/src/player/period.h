#ifndef ARCTRACKER_PERIOD_H
#define ARCTRACKER_PERIOD_H

#include <stdbool.h>

static const int PERIOD_MAX = 0x06B0;
static const int PERIOD_MIN = 0x0032;
static const int LOWEST_NOTE = 0;
static const int HIGHEST_NOTE = 61;

bool note_out_of_range(int note);

int period_for_note(int note, double fine_tuning);

int nearest_note_period(int period, double fine_tuning);

#endif //ARCTRACKER_PERIOD_H
