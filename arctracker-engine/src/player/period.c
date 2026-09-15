#include "period.h"
#include <math.h>
#include <stdlib.h>
#include "io/error.h"

static float periods[1 + HIGHEST_NOTE - LOWEST_NOTE];

void periods_init(const double base_period)
{
    for (int i = LOWEST_NOTE; i <= HIGHEST_NOTE; i++)
    {
        const double period = base_period / pow(2.0, (double) i / 12.0);
        periods[i] = (float) period;
    }
}

bool note_out_of_range(const int note)
{
    return note < LOWEST_NOTE || note > HIGHEST_NOTE;
}

int16_t period_for_note(const int note, const double fine_tuning)
{
    if (note_out_of_range(note)) return 0;
    return (int16_t) lround(periods[note] * fine_tuning);
}

int nearest_note_period(const int period, const double fine_tuning)
{
    int nearest_period = period_for_note(LOWEST_NOTE, fine_tuning);
    int nearest_distance = abs(period - nearest_period);
    for (int note = LOWEST_NOTE + 1; note <= HIGHEST_NOTE; note++)
    {
        const int candidate = period_for_note(note, fine_tuning);
        const int distance = abs(period - candidate);
        if (distance < nearest_distance)
        {
            nearest_period = candidate;
            nearest_distance = distance;
        }
    }
    return nearest_period;
}
