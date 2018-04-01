#include <stdlib.h>
#include "error.h"

#define NO_PHASE_INCREMENTS 2047
#define PHASE_INCREMENTS_LENGTH_BYTES NO_PHASE_INCREMENTS * sizeof(long)

long phase_increment(int p_period, long p_sample_rate)
{
    double period = (double) p_period;
    double sample_rate = (double) p_sample_rate;

    return 60000.0 * 3575872.0/(period * sample_rate);
}

long* calculate_phase_increments(long p_sample_rate)
{
	long* phase_increments = (long*) malloc(PHASE_INCREMENTS_LENGTH_BYTES);
	if (phase_increments == NULL)
		error(MALLOC_FAILED);

	for (int i=0; i<NO_PHASE_INCREMENTS; i++)
		phase_increments[i] = phase_increment(i + 1, p_sample_rate);

	return phase_increments;
}
