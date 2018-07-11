#include "clock.h"

static int speed;
static int sample_rate;
static int ticks;
static double frames_per_tick;
static double frames_to_be_written;

void set_clock(const int initial_speed, const int sample_rate_in)
{
    speed = initial_speed;
    sample_rate = sample_rate_in;
    frames_per_tick = sample_rate_in / DEFAULT_TICKS_PER_SECOND;
    ticks = -1;
}

void clock_tick()
{
    ticks += 1;
    frames_to_be_written += frames_per_tick;
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

void set_ticks_per_second(const int new_ticks_per_second)
{
    frames_per_tick = sample_rate / (double) new_ticks_per_second;
}

int frames_to_write()
{
    return (int) frames_to_be_written;
}

void all_frames_written()
{
    frames_to_be_written -= (int) frames_to_be_written;
}