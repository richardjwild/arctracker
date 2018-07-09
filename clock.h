#ifndef ARCTRACKER_CLOCK_H
#define ARCTRACKER_CLOCK_H

#include "arctracker.h"

void set_clock(int initial_speed, int sample_rate);

void clock_tick();

bool new_event();

void set_speed(int new_speed);

void set_speed_fine(long new_sample_periods_per_tick);

#endif //ARCTRACKER_CLOCK_H
