#include "clock.h"

static const float DEFAULT_TICKS_PER_SECOND = 50.0;

static int ticks_per_event;
static int sample_rate;
static int ticks;
static double frames_per_tick;
static double frames_to_be_written;

void set_clock(const int initial_ticks_per_event, const int sample_rate_in)
{
    ticks_per_event = initial_ticks_per_event;
    sample_rate = sample_rate_in;
    frames_per_tick = sample_rate_in / DEFAULT_TICKS_PER_SECOND;
    ticks = -1;
}

void clock_tick()
{
    ticks += 1;
    frames_to_be_written += frames_per_tick;
    if (ticks == ticks_per_event)
        ticks = 0;
}

bool new_event()
{
    return (ticks == 0);
}

void set_ticks_per_event(const int new_ticks_per_event)
{
    ticks_per_event = new_ticks_per_event;
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