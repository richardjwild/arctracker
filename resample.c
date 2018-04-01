#include <stdlib.h>
#include "error.h"

long* initialise_phase_incrementor_values(
	unsigned int *p_periods,
	long p_sample_rate)
{
	int array_bytes = 2048 * sizeof(long);

	long* phase_incrementors = (long *) malloc(array_bytes);
	if (phase_incrementors == NULL)
		error("Cannot allocate memory for phase incrementors array");

	for (int period=1; period<2048; period++)
		phase_incrementors[period - 1] = (3575872.0/((double)period * (double)p_sample_rate)) * 60000.0;

	return phase_incrementors;
}

