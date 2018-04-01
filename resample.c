#include <stdlib.h>
#include "heap.h"

#define NO_PHASE_INCREMENTS 2047

long phase_increment(int p_period, long p_sample_rate)
{
    double period = (double) p_period;
    double sample_rate = (double) p_sample_rate;

    return 60000.0 * 3575872.0/(period * sample_rate);
}

long* calculate_phase_increments(long p_sample_rate)
{
	long* phase_increments = (long*) allocate_array(NO_PHASE_INCREMENTS, sizeof(long));

	for (int i=0; i<NO_PHASE_INCREMENTS; i++)
		phase_increments[i] = phase_increment(i + 1, p_sample_rate);

	return phase_increments;
}
