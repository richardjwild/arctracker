#include <stdlib.h>
#include "heap.h"

#define PITCH_QUANTA 2047
#define PHASE_INCREMENT_CONVERSION 60000L * 3575872L

long* phase_increments;

long phase_increment(int period, long sample_rate)
{
    return PHASE_INCREMENT_CONVERSION/(period * sample_rate);
}

void calculate_phase_increments(long sample_rate)
{
    phase_increments = (long*) allocate_array(PITCH_QUANTA, sizeof(long));

    for (int period=1; period<=PITCH_QUANTA; period++)
        phase_increments[period - 1] = phase_increment(period, sample_rate);
}

long phase_increment_for(int period)
{
    return phase_increments[period];
}
