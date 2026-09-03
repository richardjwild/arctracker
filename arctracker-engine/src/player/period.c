#include "period.h"
#include <math.h>
#include <stdlib.h>
#include "io/error.h"

static const float periods[] = {
    1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017,  961,  907,
     856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  453,
     428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240,  226,
     214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120,  113,
     107,  101,   95,   90,   85,   80,   76,   71,   67,   64,   60,   57,
      53,   50,
};

bool note_out_of_range(const int note)
{
    return note < LOWEST_NOTE || note > HIGHEST_NOTE;
}

int period_for_note(const int note, const double fine_tuning)
{
    if (note_out_of_range(note)) return 0;
    return (int) lround(periods[note] * fine_tuning);
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
