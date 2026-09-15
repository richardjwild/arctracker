#ifndef ARCTRACKER_PERIOD_H
#define ARCTRACKER_PERIOD_H

#include <stdbool.h>
#include <stdint.h>

#define PERIOD_MAX 1712.0
#define PERIOD_MIN 50.0
#define LOWEST_NOTE 0
#define HIGHEST_NOTE 61

void periods_init(double base_period);

bool note_out_of_range(int note);

int16_t period_for_note(int note, double fine_tuning);

int nearest_note_period(int period, double fine_tuning);

#endif //ARCTRACKER_PERIOD_H
