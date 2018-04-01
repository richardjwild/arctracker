#include <stdlib.h>
#include "heap.h"

#define NO_PHASE_INCREMENTS 2047

long* phase_increments;

long phase_increment(int p_period, long p_sample_rate)
{
    double period = (double) p_period;
    double sample_rate = (double) p_sample_rate;

    return 60000.0 * 3575872.0/(period * sample_rate);
}

void calculate_phase_increments(long p_sample_rate)
{
	phase_increments = (long*) allocate_array(NO_PHASE_INCREMENTS, sizeof(long));

	for (int period=1; period<=NO_PHASE_INCREMENTS; period++)
		phase_increments[period - 1] = phase_increment(period, p_sample_rate);
}

long phase_increment_for(int period)
{
    return phase_increments[period];
}
