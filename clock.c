#include "clock.h"

static int speed;
static int ticks;
static long sample_periods_per_tick;

void set_clock(int initial_speed, int sample_rate)
{
    speed = initial_speed;
    sample_periods_per_tick = (sample_rate << 8) / 50;
    ticks = -1;
}

void clock_tick()
{
    ticks += 1;
    if (ticks == speed)
        ticks = 0;
}

bool new_event()
{
    return (ticks == 0);
}

void set_speed(const int new_speed)
{
    speed = new_speed;
}

void set_speed_fine(const long new_sample_periods_per_tick)
{
    sample_periods_per_tick = new_sample_periods_per_tick;
}