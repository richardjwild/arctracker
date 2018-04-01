#include <stdlib.h>
#include "error.h"

long calculate_phase_increment(int p_period, long p_sample_rate)
{
    double period = (double) p_period;
    double sample_rate = (double) p_sample_rate;

    return 60000.0 * 3575872.0/(period * sample_rate);
}

long* initialise_phase_incrementor_values(long p_sample_rate)
{
	int array_bytes = 2048 * sizeof(long);

	long* phase_incrementors = (long *) malloc(array_bytes);
	if (phase_incrementors == NULL)
		error("Cannot allocate memory for phase incrementors array");

	for (int period=1; period<2048; period++)
		phase_incrementors[period - 1] = calculate_phase_increment(period, p_sample_rate);

	return phase_incrementors;
}

