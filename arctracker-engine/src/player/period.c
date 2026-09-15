#include "period.h"
#include <math.h>
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

float period_for_note(const int note, const double fine_tuning)
{
    if (note_out_of_range(note)) return 0;
    return periods[note] * (float) fine_tuning;
}

float nearest_note_period(const float period, const double fine_tuning)
{
    float nearest_period = period_for_note(LOWEST_NOTE, fine_tuning);
    float nearest_distance = fabsf(period - nearest_period);
    for (int note = LOWEST_NOTE + 1; note <= HIGHEST_NOTE; note++)
    {
        const float candidate = period_for_note(note, fine_tuning);
        const float distance = fabsf(period - candidate);
        if (distance < nearest_distance)
        {
            nearest_period = candidate;
            nearest_distance = distance;
        }
    }
    return nearest_period;
}
