#ifndef ARCTRACKER_CLOCK_H
#define ARCTRACKER_CLOCK_H

#include <stdbool.h>

void set_clock(int initial_ticks_per_event, int sample_rate_in);

void clock_tick();

bool new_event();

void set_ticks_per_event(int new_ticks_per_event);

void set_ticks_per_second(int new_ticks_per_second);

int frames_to_write();

void all_frames_written();

#endif //ARCTRACKER_CLOCK_H
